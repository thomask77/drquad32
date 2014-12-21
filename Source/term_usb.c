#include "term_usb.h"
#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include "ustime.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

#define RX_QUEUE_SIZE  1024
#define TX_QUEUE_SIZE  1024

static volatile struct usb_stats {
    uint32_t    irq_count;
    uint32_t    rx_bytes;
    uint32_t    tx_bytes;
    uint32_t    rx_overrun;
    uint32_t    tx_overrun;
} usb_stats;

static QueueHandle_t rx_queue;
static QueueHandle_t tx_queue;


// -------------------- USB CDC Functions --------------------
//
static USB_OTG_CORE_HANDLE  USB_OTG_dev;

extern uint8_t  APP_Rx_Buffer[APP_RX_DATA_SIZE];
extern uint32_t APP_Rx_ptr_in;

static uint16_t VCP_Init(void)
{
    return USBD_OK;
}

static uint16_t VCP_DeInit(void)
{
    return USBD_OK;
}

static uint16_t VCP_Ctrl(uint32_t Cmd, uint8_t *Buf, uint32_t Len)
{
    return USBD_OK;
}


static uint16_t VCP_DataTx(uint8_t *buf, uint32_t len)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    for (int i=0; i<uxQueueMessagesWaitingFromISR(tx_queue); i++)
    {
        char c;
        xQueueReceiveFromISR(tx_queue, &c, &xHigherPriorityTaskWoken);

        APP_Rx_Buffer[APP_Rx_ptr_in] = c;
        APP_Rx_ptr_in++;

        if (APP_Rx_ptr_in == APP_RX_DATA_SIZE)
            APP_Rx_ptr_in = 0;

        usb_stats.tx_bytes++;
    }

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);

    return USBD_OK;
}


static uint16_t VCP_DataRx(uint8_t *buf, uint32_t len)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    for (int i=0; i<len; i++) {
        if (xQueueSendFromISR(rx_queue, &buf[i], &xHigherPriorityTaskWoken))
            usb_stats.rx_bytes++;
        else
            usb_stats.rx_overrun++;
    }

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);

    return USBD_OK;
}

const CDC_IF_Prop_TypeDef VCP_fops = {
    .pIf_Init   = VCP_Init,
    .pIf_DeInit = VCP_DeInit,
    .pIf_Ctrl   = VCP_Ctrl,
    .pIf_DataTx = VCP_DataTx,
    .pIf_DataRx = VCP_DataRx
};


// -------------------- Character Device Functions --------------------
//
static ssize_t usb_write_r(struct _reent *r, int fd, const void *ptr, size_t len)
{
    const char *src = ptr;
    int sent = 0;

    while (len--) {
        // Convert c-newlines to terminal CRLF
        //
        if (*src == '\n')
            xQueueSend(tx_queue, "\r", portMAX_DELAY);

        xQueueSend(tx_queue, src++, portMAX_DELAY);
        sent++;
    }

    return sent;
}

static ssize_t usb_read_r(struct _reent *r, int fd, void *ptr, size_t len)
{
    // Blocking wait for the first char
    //
    unsigned timeout = portMAX_DELAY;

    char *dest = ptr;
    int  received = 0;
    static char last_c;

    while (len--) {
        char c;
        if (!xQueueReceive(rx_queue, &c, timeout))
            break;

        // Convert terminal CRLF to c-newline
        //
        if (c == '\n' && last_c == '\r')
            if (!xQueueReceive(rx_queue, &c, timeout))
                break;

        if (c == '\r')
            *dest++ = '\n';
        else
            *dest++ = c;

        last_c  = c;
        timeout = 0;
        received++;
    }

    return received;
}

static ssize_t usb_chars_avail_r(struct _reent *r, int fd)
{
    return uxQueueMessagesWaiting(rx_queue);
}


struct  file_ops  term_usb_ops = {
    .read_r        = usb_read_r,
    .write_r       = usb_write_r,
    .chars_avail_r = usb_chars_avail_r
};


void term_usb_init(void)
{
    rx_queue = xQueueCreate(RX_QUEUE_SIZE, 1);
    tx_queue = xQueueCreate(TX_QUEUE_SIZE, 1);

    USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb, &USR_cb);

    dev_register("usb", &term_usb_ops);
}


// -------------------- USB BSP Functions --------------------
//
#include "usb_bsp.h"
#include "usb_dcd_int.h"
#include "board.h"

void OTG_FS_IRQHandler(void)
{
    USBD_OTG_ISR_Handler(&USB_OTG_dev);
    VCP_DataTx(0, 0);
    usb_stats.irq_count++;
}

void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE *pdev)
{
    // Enable peripheral clocks
    //
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB2ENR |= RCC_APB2ENR_SYSCFGEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;

    // Configure DM and DP Pins
    //
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_OTG1_FS);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_OTG1_FS);

    GPIO_Init(GPIOA, &(GPIO_InitTypeDef ) {
        .GPIO_Pin   = GPIO_Pin_11 | GPIO_Pin_12,
        .GPIO_Speed = GPIO_Speed_50MHz,
        .GPIO_Mode  = GPIO_Mode_AF,
        .GPIO_OType = GPIO_OType_PP,
        .GPIO_PuPd  = GPIO_PuPd_NOPULL
    });
}

void USB_OTG_BSP_EnableInterrupt(USB_OTG_CORE_HANDLE *pdev)
{
    NVIC_Init(&(NVIC_InitTypeDef) {
        .NVIC_IRQChannel = OTG_FS_IRQn,
        .NVIC_IRQChannelPreemptionPriority =
                configLIBRARY_LOWEST_INTERRUPT_PRIORITY,
        .NVIC_IRQChannelSubPriority = 0,
        .NVIC_IRQChannelCmd = ENABLE
    });
}

void USB_OTG_BSP_uDelay(const uint32_t usec)
{
    delay_us(usec);
}

void USB_OTG_BSP_mDelay(const uint32_t msec)
{
    delay_ms(msec);
}

// -------------------- Shell commands --------------------
//
#include "command.h"

static void cmd_usb_stats(void)
{
    printf("irq_count:  %10lu\n", usb_stats.irq_count  );
    printf("rx_bytes:   %10lu\n", usb_stats.rx_bytes   );
    printf("rx_overrun: %10lu\n", usb_stats.rx_overrun );
    printf("tx_bytes:   %10lu\n", usb_stats.tx_bytes   );
    printf("tx_overrun: %10lu\n", usb_stats.tx_overrun );
}

SHELL_CMD(usb_stats, (cmdfunc_t)cmd_usb_stats, "show USB statistics")
