#include "spi.h"
#include "FreeRTOS.h"
#include "semphr.h"

#define RX_DMA            DMA1_Stream3  // Channel 0: SPI2_RX
#define TX_DMA            DMA1_Stream4  // Channel 0: SPI2_TX

static xSemaphoreHandle   spi_sem;
static xSemaphoreHandle   dma_sem;
static GPIO_TypeDef      *cs_gpio;      // Current chip select GPIO
static int                cs_pin;       // Current chip select Pin

static void init_spi(SPI_InitTypeDef spiInit)
{
    // Initialize SPI peripheral
    //
    SPI_Init(SPI2, &spiInit );
    SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Rx, ENABLE);
    SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
    SPI_Cmd(SPI2, ENABLE);
}

/**
 * SPI2 receive DMA complete interrupt
 *
 */
void DMA1_Stream3_IRQHandler(void)
{
    DMA_ClearITPendingBit(DMA1_Stream3, DMA_IT_TCIF3);
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(dma_sem, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}


/**
 * Begin a SPI transaction.
 *
 * \param  gpio      Chip select GPIO
 * \param  pin       Chip select pin
 * \param  spiPol    SPI polarity
 * \param  spiChase  SPI phase
 */
void spi_begin(GPIO_TypeDef *gpio, int pin, SPI_InitTypeDef spiInit)
{
    xSemaphoreTake(spi_sem, portMAX_DELAY);

    init_spi(spiInit);
    cs_gpio = gpio;
    cs_pin  = pin;
    GPIO_ResetBits(cs_gpio, cs_pin);
}


/**
 * Send data bytes over SPI.
 *
 * \param  out   Pointer to output buffer
 * \param  in    Pointer to input buffer
 * \param  len   Length of transfer
 *
 * [in] and [out] may point to overlapping memory areas.
 * One of them can be NULL for write- or read-only transfers.
 *
 */
void spi_transfer(const void *out, void *in, int len)
{
    static char  in_dummy, out_dummy;

    // Set up DMA channels
    //
    RX_DMA->CR   = TX_DMA->CR   = 0;
    RX_DMA->NDTR = TX_DMA->NDTR = len;
    RX_DMA->PAR  = TX_DMA->PAR  = (uint32_t)&SPI2->DR;

    // Start DMA channels
    //
    if (in) {
        // SPI -> *in++, TCIE
        RX_DMA->M0AR = (uint32_t)in;
        RX_DMA->CR   = DMA_SxCR_MINC | DMA_SxCR_TCIE | DMA_SxCR_EN;
    }
    else {
        // SPI -> in_dummy, TCIE
        RX_DMA->M0AR = (uint32_t)&in_dummy;
        RX_DMA->CR   = DMA_SxCR_TCIE | DMA_SxCR_EN;
    }

    if (out) {
        // *out++ -> SPI
        TX_DMA->M0AR = (uint32_t)out;
        TX_DMA->CR   = DMA_SxCR_MINC | DMA_SxCR_DIR_0 | DMA_SxCR_EN;
    }
    else {
        // out_dummy -> SPI
        TX_DMA->M0AR = (uint32_t)&out_dummy;
        TX_DMA->CR   = DMA_SxCR_DIR_0 | DMA_SxCR_EN;
    }

    // Wait for rx_dma to finish
    //
    xSemaphoreTake(dma_sem, portMAX_DELAY);

    // Clear flags
    //
    // TODO: For some reason, the TX_DMA stream
    // always signals an error on end-of-transfer
    // (Bit 0 in DMA1->HISR, DMA_HISR_FEIF4)
    //
    // printf("LISR:  %08x\n", DMA1->LISR);
    // printf("HISR:  %08x\n", DMA1->HISR);
    //
    DMA1->LIFCR = DMA1->LISR;
    DMA1->HIFCR = DMA1->HISR;
}


/**
 * End a SPI transaction.
 *
 */
void spi_end()
{
    GPIO_SetBits(cs_gpio, cs_pin);
    xSemaphoreGive(spi_sem);
}


/**
 * Initialize the SPI interfaces
 *
 */
void spi_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_DMA1;
    RCC->APB1ENR |= RCC_APB1Periph_SPI2;

    // Create synchronization objects
    // Use a mutex (w/ priority inheritance) for the SPI channel
    //
    spi_sem = xSemaphoreCreateMutex();

    // Create and initialize a normal semaphore for DMA
    // (xSemaphoreGiveFromISR doesn't work with mutexes)
    //
    dma_sem = xSemaphoreCreateBinary();

    // SPI2_CLK   PB10
    // SPI2_MISO  PC2
    // SPI2_MOSI  PC3
    //
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_SPI2);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource2, GPIO_AF_SPI2);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource3, GPIO_AF_SPI2);

    GPIO_Init(GPIOB, &(GPIO_InitTypeDef) {
        .GPIO_Pin   = GPIO_Pin_10,
        .GPIO_Mode  = GPIO_Mode_AF,
        .GPIO_Speed = GPIO_Speed_50MHz,
        .GPIO_OType = GPIO_OType_PP,
        .GPIO_PuPd  = GPIO_PuPd_DOWN
    });

    GPIO_Init(GPIOC, &(GPIO_InitTypeDef) {
        .GPIO_Pin   = GPIO_Pin_2,
        .GPIO_Mode  = GPIO_Mode_AF,
        .GPIO_Speed = GPIO_Speed_50MHz,
        .GPIO_OType = GPIO_OType_PP,
        .GPIO_PuPd  = GPIO_PuPd_DOWN
    });

    GPIO_Init(GPIOC, &(GPIO_InitTypeDef) {
        .GPIO_Pin   = GPIO_Pin_3,
        .GPIO_Mode  = GPIO_Mode_AF,
        .GPIO_PuPd  = GPIO_PuPd_DOWN
    });

    // Initialize DMA channels and interrupts
    //
    NVIC_Init(&(NVIC_InitTypeDef){
        .NVIC_IRQChannel = DMA1_Stream3_IRQn,
        .NVIC_IRQChannelPreemptionPriority =
                configLIBRARY_LOWEST_INTERRUPT_PRIORITY,
        .NVIC_IRQChannelSubPriority = 0,
        .NVIC_IRQChannelCmd = ENABLE
    });
}
