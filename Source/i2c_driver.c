#include "i2c_driver.h"
#include "util.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define I2C_SPEED   400000  // [bits/s]

// #define I2C_DEBUG

#ifdef I2C_DEBUG

static struct {
    uint16_t    t;
    enum  {
        I2C_LOG_EVT,
        I2C_LOG_ERR,
        I2C_LOG_RXTC,
        I2C_LOG_TXTC
    } ev;
    uint16_t  sr1, sr2;
}  i2c_log[16];

static int i2c_log_cur;

static void i2c_log_event(int ev, uint16_t sr1, uint16_t sr2)
{
    if (i2c_log_cur < ARRAY_SIZE(i2c_log)) {
        i2c_log[i2c_log_cur].t   = TIM7->CNT;
        i2c_log[i2c_log_cur].ev  = ev;
        i2c_log[i2c_log_cur].sr1 = sr1;
        i2c_log[i2c_log_cur].sr2 = sr2;
        i2c_log_cur++;
    }
}

static void i2c_show_log(void)
{
    uint16_t t0 = i2c_log[0].t;

    for (int i=0; i<i2c_log_cur; i++) {
        printf("%4d %s: %04x %04x%s%s%s%s%s%s%s %s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
            (uint16_t)(i2c_log[i].t - t0),
            i2c_log[i].ev == I2C_LOG_EVT    ? "EVT " :
            i2c_log[i].ev == I2C_LOG_ERR    ? "ERR " :
            i2c_log[i].ev == I2C_LOG_RXTC   ? "RXTC" :
            i2c_log[i].ev == I2C_LOG_TXTC   ? "TXTC" : "???",
            i2c_log[i].sr2, i2c_log[i].sr1,
            i2c_log[i].sr2 & I2C_SR2_DUALF      ? " DUALF"      : "",
            i2c_log[i].sr2 & I2C_SR2_SMBHOST    ? " SMBHOST"    : "",
            i2c_log[i].sr2 & I2C_SR2_SMBDEFAULT ? " SMBDEFAULT" : "",
            i2c_log[i].sr2 & I2C_SR2_GENCALL    ? " GENCALL"    : "",
            i2c_log[i].sr2 & I2C_SR2_TRA        ? " TRA"        : "",
            i2c_log[i].sr2 & I2C_SR2_BUSY       ? " BUSY"       : "",
            i2c_log[i].sr2 & I2C_SR2_MSL        ? " MSL"        : "",
            i2c_log[i].sr1 & I2C_SR1_SMBALERT   ? " SMBALERT"   : "",
            i2c_log[i].sr1 & I2C_SR1_TIMEOUT    ? " TIMEOUT"    : "",
            i2c_log[i].sr1 & I2C_SR1_PECERR     ? " PECERR"     : "",
            i2c_log[i].sr1 & I2C_SR1_OVR        ? " OVR"        : "",
            i2c_log[i].sr1 & I2C_SR1_AF         ? " AF"         : "",
            i2c_log[i].sr1 & I2C_SR1_ARLO       ? " ARLO"       : "",
            i2c_log[i].sr1 & I2C_SR1_BERR       ? " BERR"       : "",
            i2c_log[i].sr1 & I2C_SR1_TXE        ? " TXE"        : "",
            i2c_log[i].sr1 & I2C_SR1_RXNE       ? " RXNE"       : "",
            i2c_log[i].sr1 & I2C_SR1_STOPF      ? " STOPF"      : "",
            i2c_log[i].sr1 & I2C_SR1_ADD10      ? " ADD10"      : "",
            i2c_log[i].sr1 & I2C_SR1_BTF        ? " BTF"        : "",
            i2c_log[i].sr1 & I2C_SR1_ADDR       ? " ADDR"       : "",
            i2c_log[i].sr1 & I2C_SR1_SB         ? " SB"         : ""
        );
    }

    i2c_log_cur = 0;
}

#else

#define i2c_log_event(...)
#define i2c_show_log()

#endif


static struct {
    uint32_t    rx_bytes;
    uint32_t    tx_bytes;
    uint32_t    naks;
    uint32_t    timeouts;
    uint32_t    arlo, berr;
} i2c_stats;


static SemaphoreHandle_t  i2c_mutex;
static SemaphoreHandle_t  i2c_irq_sem;

static volatile uint8_t   s_addr;


static void i2c_stop(void)
{
    // Send STOP condition
    //
    I2C1->CR1 |= I2C_CR1_STOP;

    // Disable interrupts and DMA channels
    //
    I2C1->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
    DMA1_Stream0->CR &= ~(DMA_SxCR_TCIE | DMA_SxCR_EN);
    DMA1_Stream7->CR &= ~(DMA_SxCR_TCIE | DMA_SxCR_EN);

    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(i2c_irq_sem, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}


void DMA1_Stream0_IRQHandler(void)
{
    i2c_log_event(I2C_LOG_RXTC, I2C1->SR1, I2C1->SR2);
    i2c_stop();
}


void I2C1_ER_IRQHandler(void)
{
    i2c_log_event(I2C_LOG_ERR, I2C1->SR1, I2C1->SR2);
    i2c_stop();
}


void I2C1_EV_IRQHandler(void)
{
    uint16_t  sr1 = I2C1->SR1;
    i2c_log_event(I2C_LOG_EVT, sr1, 0);

    if (sr1 & I2C_SR1_SB) {
        // Clear flag by sending device address
        //
        I2C1->DR = s_addr;
    }

    if (sr1 & I2C_SR1_ADDR) {
        int len = (s_addr & 1) ? DMA1_Stream0->NDTR : DMA1_Stream7->NDTR;
        if (len == 0) {
            // Zero length transfer, just probing for address
            //
            i2c_stop();
        }

        // Clear by reading SR2
        //
        I2C1->SR2;
    }

    if (sr1 & I2C_SR1_BTF) {
        // Transmit DMA finished, disable IRQs
        //
        i2c_stop();
    }

    I2C1->CR1;  // Dummy read to prevent IRQ glitches
}


static int i2c_transfer(uint8_t addr, void *buf, size_t len)
{
    if (addr & 0x01) {
        // Set up master-receiver DMA
        //
        DMA1_Stream0->CR  &= ~DMA_SxCR_EN;
        while (DMA1_Stream0->CR  & DMA_SxCR_EN);

        DMA1->LIFCR = DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0  |
                      DMA_LIFCR_CTEIF0 | DMA_LIFCR_CDMEIF0 |
                      DMA_LIFCR_CFEIF0;

        DMA1_Stream0->PAR  = (uint32_t)&I2C1->DR;
        DMA1_Stream0->M0AR = (uint32_t)buf;
        DMA1_Stream0->NDTR = len;
        DMA1_Stream0->CR   = DMA_Channel_1 | DMA_SxCR_MINC | DMA_SxCR_TCIE;
        DMA1_Stream0->CR  |= DMA_SxCR_EN;

        if (len > 1)
            I2C1->CR1 |=  I2C_CR1_ACK;
        else
            I2C1->CR1 &= ~I2C_CR1_ACK;
    }
    else {
        // Set up master-transmitter DMA
        //
        DMA1_Stream7->CR  &= ~DMA_SxCR_EN;
        while (DMA1_Stream7->CR  & DMA_SxCR_EN);

        DMA1->HIFCR = DMA_HIFCR_CTCIF7 | DMA_HIFCR_CHTIF7  |
                      DMA_HIFCR_CTEIF7 | DMA_HIFCR_CDMEIF7 |
                      DMA_HIFCR_CFEIF7;

        DMA1_Stream7->PAR  = (uint32_t)&I2C1->DR;
        DMA1_Stream7->M0AR = (uint32_t)buf;
        DMA1_Stream7->NDTR = len;
        DMA1_Stream7->CR   = DMA_Channel_1 | DMA_SxCR_MINC | DMA_SxCR_DIR_0;
        DMA1_Stream7->CR  |= DMA_SxCR_EN;
    }

    // Send START condition
    //
    s_addr = addr;
    I2C1->CR2 |= I2C_CR2_LAST | I2C_CR2_DMAEN;
    I2C1->CR2 |= I2C_CR2_ITEVTEN | I2C_CR2_ITERREN;
    I2C1->CR1 |= I2C_CR1_START;

    // Wait for the transfer to finish
    //
    TickType_t  timeout = 2 * (len * 9 * configTICK_RATE_HZ / I2C_SPEED + 1);
    TickType_t  t0 = xTaskGetTickCount();

    int res = xSemaphoreTake(i2c_irq_sem, timeout);

    while (I2C1->SR2 & I2C_SR2_BUSY) {
        if (xTaskGetTickCount() - t0 > timeout) {
            res = pdFAIL;
            break;
        }
    }

    uint16_t sr1 = I2C1->SR1;

    if (res != pdPASS)          i2c_stats.timeouts++;
    if (sr1 & I2C_SR1_ARLO)     i2c_stats.arlo++;
    if (sr1 & I2C_SR1_BERR)     i2c_stats.berr++;
    if (I2C1->SR1 & I2C_SR1_AF) i2c_stats.naks++;

    if (res != pdPASS || sr1 & I2C_SR1_ARLO || sr1 & I2C_SR1_BERR) {
        // Something went wrong on the bus.. try to reinitialize.
        //
        i2c_stop();
        i2c_init();
        errno = EBUSY;
        return -1;
    }

    if (I2C1->SR1 & I2C_SR1_AF) {
        // Slave address not acknowledged
        //
        I2C1->SR1 = 0;
        errno = ENXIO;
        return -1;
    }

    if (addr & 1)
        i2c_stats.rx_bytes += len;
    else
        i2c_stats.tx_bytes += len;

    return len;
}


int i2c_read(uint8_t addr, uint8_t reg, void *data, size_t size)
{
    xSemaphoreTake(i2c_mutex, portMAX_DELAY);

    int res = i2c_transfer(addr & 0xFE, &reg, 1);
    if (res == 1)
        res = i2c_transfer(addr | 0x01, data, size);

    i2c_show_log();
    xSemaphoreGive(i2c_mutex);

    return res;
}


int i2c_write(uint8_t addr, uint8_t reg, const void *data, size_t size)
{
    uint8_t reg_data[size + 1];

    reg_data[0] = reg;
    memcpy(&reg_data[1], data, size);

    xSemaphoreTake(i2c_mutex, portMAX_DELAY);

    int res = i2c_transfer(addr & 0xFE, &reg_data, size+1);

    i2c_show_log();
    xSemaphoreGive(i2c_mutex);

    return res;
}


void i2c_init(void)
{
    // Enable peripheral clocks
    //
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    I2C_DeInit(I2C1);
    DMA_DeInit(DMA1_Stream0);   // RX_DMA
    DMA_DeInit(DMA1_Stream7);   // TX_DMA

    // Initialize I2C peripheral
    //
    I2C_Init(I2C1, &(I2C_InitTypeDef) {
        .I2C_ClockSpeed          = I2C_SPEED,
        .I2C_Mode                = I2C_Mode_I2C,
        .I2C_DutyCycle           = I2C_DutyCycle_16_9,
        .I2C_OwnAddress1         = 0xAA,
        .I2C_Ack                 = I2C_Ack_Enable,
        .I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit
    });

    // PB6  I2C1_SCL
    // PB7  I2C1_SDA
    //
    GPIO_SetBits(GPIOB, GPIO_Pin_6 | GPIO_Pin_7);

    GPIO_InitTypeDef gpio = {
        .GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7,
        .GPIO_Mode  = GPIO_Mode_OUT,
        .GPIO_Speed = GPIO_Low_Speed,
        .GPIO_OType = GPIO_OType_OD,
        .GPIO_PuPd  = GPIO_PuPd_UP
    };

    GPIO_Init(GPIOB, &gpio);

    // Clear the bus by reading 8 data bits + NAK
    //
    for (int i=0; i<9; i++) {
        GPIO_SetBits(GPIOB, GPIO_Pin_6);
        delay_us(2);
        GPIO_ResetBits(GPIOB, GPIO_Pin_6);
        delay_us(2);
    }

    // Send STOP condition
    //
    GPIO_ResetBits(GPIOB, GPIO_Pin_7);
    delay_us(2);
    GPIO_SetBits(GPIOB, GPIO_Pin_6);
    delay_us(2);
    GPIO_SetBits(GPIOB, GPIO_Pin_7);
    delay_us(2);

    // Configure GPIOs for alternate function
    //
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1);

    gpio.GPIO_Mode = GPIO_Mode_AF;
    GPIO_Init(GPIOB, &gpio);

    // Set up event and error interrupts
    //
    NVIC_InitTypeDef nvic = {
        .NVIC_IRQChannelPreemptionPriority =
                configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY,
        .NVIC_IRQChannelCmd = ENABLE
    };

    nvic.NVIC_IRQChannel = I2C1_EV_IRQn;
    NVIC_Init(&nvic);

    nvic.NVIC_IRQChannel = I2C1_ER_IRQn;
    NVIC_Init(&nvic);

    nvic.NVIC_IRQChannel = DMA1_Stream0_IRQn;
    NVIC_Init(&nvic);

    // Create a mutex for the read/write API and a normal semaphore for IRQs
    // (xSemaphoreGiveFromISR doesn't work with mutexes)
    //
    if (!i2c_mutex)   i2c_mutex   = xSemaphoreCreateMutex();
    if (!i2c_irq_sem) i2c_irq_sem = xSemaphoreCreateBinary();

    xSemaphoreTake(i2c_irq_sem, 0);
    xSemaphoreGive(i2c_mutex);

    I2C_Cmd(I2C1, ENABLE);
}


// -------------------- Shell commands --------------------
//
#include "command.h"


static void cmd_i2c_scan(void)
{
    printf("scanning..\n");

    // Scan all valid slave addresses
    //
    int n = 0;
    for (int addr=0x10; addr<0xF0; addr+=2)
    {
        xSemaphoreTake(i2c_mutex, portMAX_DELAY);
        int res = i2c_transfer(addr, NULL, 0);
        i2c_show_log();
        xSemaphoreGive(i2c_mutex);

        if (res >= 0) {
            printf("  slave found at 0x%02x.\n", addr);
            n++;
        }
    }

    printf("%d slaves found.\n", n);
}


static void cmd_i2c_stats(void)
{
    printf("rx_bytes:   %10lu\n", i2c_stats.rx_bytes  );
    printf("tx_bytes:   %10lu\n", i2c_stats.tx_bytes  );
    printf("naks:       %10lu\n", i2c_stats.naks      );
    printf("timeouts:   %10lu\n", i2c_stats.timeouts  );
    printf("arlo:       %10lu\n", i2c_stats.arlo      );
    printf("berr:       %10lu\n", i2c_stats.berr      );
}

SHELL_CMD(i2c_scan, (cmdfunc_t)cmd_i2c_scan, "Scan the I2C bus")
SHELL_CMD(i2c_stats, (cmdfunc_t)cmd_i2c_stats, "Show I2C statistics")
