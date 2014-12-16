#include "bldc_driver.h"
#include "bldc_task.h"
#include "util.h"
#include "gamma_tab.inc"
#include "stm32f4xx.h"
#include "FreeRTOS.h"

// Pinout
// ======
//
//  FL    A B C FR
//  C
//  B     z Y
//  A      \|    A
//       X--o    B
//               C
//  RL C B A    RR
//
//
// Block commutation scheme
// ========================
//
//       B         .            1   2   3   4   5   6
//        \   2   .             aaaabbbbbbbbccccccccaaaa
//         \     .                 ba      cb      ac
//     3    \   .    1            b  a    c  b    a  c
//           \ .                 b    a  c    b  a    c
//   - - - - -0-------- A       b      ac      ba      c
//           / .                ccccccccaaaaaaaabbbbbbbb
//     4    /   .    6
//         /     .              0: Brake, 7: Free-Wheel
//        /   5   .
//       C         .
//

#define TIMEBASE_FREQ   (168000000 / 2)

#define PWM_FREQ        20000       // Hz
#define PWM_MAX_COUNT   ((TIMEBASE_FREQ / 2) / PWM_FREQ)

#define ADC_U_REF       3.3
#define ADC_MAX_COUNT   4096
#define ADC_LSB         (ADC_U_REF / ADC_MAX_COUNT)

#define ADC_NSAMPLES    10
#define ADC_FREQ        (BLDC_IRQ_FREQ * ADC_NSAMPLES)      // Hz

#define U_BAT_R1        5600.0
#define U_BAT_R2        1000.0

#define U_BAT_LSB       (ADC_LSB / (U_BAT_R2 / (U_BAT_R1 + U_BAT_R2)))

// DMA buffers may not cross 1kb boundaries while doing a burst.
//
// Make sure that the buffers are 16 byte aligned and are a
// multiple of 16 bytes.
//
// TODO: Double-Buffering
//
static uint16_t  dma_buf[3 * 4 * ADC_NSAMPLES]  __attribute__ ((aligned(16)));

STATIC_ASSERT(sizeof(dma_buf) % 16 == 0);
STATIC_ASSERT(ADC_NSAMPLES < 16);

volatile uint32_t   bldc_irq_count;
volatile uint32_t   bldc_irq_time;
volatile uint32_t   bldc_irq_time1;
volatile uint32_t   bldc_irq_time2;
volatile uint32_t   bldc_irq_time3;


// don't let gcc see this ;)
extern void adc_filter(void *s, void *d, int n);


static void bldc_get_measurements(void)
{
    bldc_state.u_bat  = ADC1->JDR1 * U_BAT_LSB;
    bldc_state.u_aux  = ADC2->JDR1 * ADC_LSB;
    bldc_state.thdn   = !(GPIOE->IDR & GPIO_Pin_15);

    uint16_t blubb[12];
    adc_filter(dma_buf, blubb, ADC_NSAMPLES);

    const float k = U_BAT_LSB / ADC_NSAMPLES;

    bldc_state.motors[ID_FL].u_a = blubb[0]  * k;
    bldc_state.motors[ID_FL].u_b = blubb[1]  * k;
    bldc_state.motors[ID_FL].u_c = blubb[2]  * k;
    bldc_state.motors[ID_FR].u_a = blubb[3]  * k;
    bldc_state.motors[ID_FR].u_b = blubb[4]  * k;
    bldc_state.motors[ID_FR].u_c = blubb[5]  * k;
    bldc_state.motors[ID_RL].u_a = blubb[9]  * k;
    bldc_state.motors[ID_RL].u_b = blubb[10] * k;
    bldc_state.motors[ID_RL].u_c = blubb[11] * k;
    bldc_state.motors[ID_RR].u_a = blubb[8]  * k;
    bldc_state.motors[ID_RR].u_b = blubb[7]  * k;
    bldc_state.motors[ID_RR].u_c = blubb[6]  * k;
}


#define ENABLE_PWM(var, pin, enable)                        \
        var = enable  ? var |   GPIO_Mode_AF << (pin * 2)   \
                      : var & ~(GPIO_Mode_AF << (pin * 2))


inline __attribute__((always_inline))
static void bldc_set_pwm( int id,
        int pwm_a, int pwm_b, int pwm_c,
        int en_a,  int en_b,  int en_c
)
{
    switch (id) {
    case ID_FL: {
        uint32_t  gpioc_moder = GPIOC->MODER;
        ENABLE_PWM(gpioc_moder, 6, en_a);
        ENABLE_PWM(gpioc_moder, 7, en_b);
        ENABLE_PWM(gpioc_moder, 8, en_c);

        GPIOC->MODER = gpioc_moder;
        TIM3->CCR1 = pwm_a;
        TIM3->CCR2 = pwm_b;
        TIM3->CCR3 = pwm_c;
        break;
    }

    case ID_FR: {
        uint32_t  gpioa_moder = GPIOA->MODER;
        uint32_t  gpiob_moder = GPIOB->MODER;
        ENABLE_PWM(gpioa_moder, 15, en_a);
        ENABLE_PWM(gpiob_moder,  3, en_b);
        ENABLE_PWM(gpiob_moder, 10, en_c);

        GPIOA->MODER = gpioa_moder;
        GPIOB->MODER = gpiob_moder;
        TIM2->CCR1 = PWM_MAX_COUNT - pwm_a;
        TIM2->CCR2 = PWM_MAX_COUNT - pwm_b;
        TIM2->CCR3 = PWM_MAX_COUNT - pwm_c;
        break;
    }

    case ID_RL: {
        uint32_t  gpiod_moder = GPIOD->MODER;
        ENABLE_PWM(gpiod_moder, 12, en_a);
        ENABLE_PWM(gpiod_moder, 13, en_b);
        ENABLE_PWM(gpiod_moder, 14, en_c);

        GPIOD->MODER = gpiod_moder;
        TIM4->CCR1 = pwm_a;
        TIM4->CCR2 = pwm_b;
        TIM4->CCR3 = pwm_c;
        break;
    }

    case ID_RR: {
        uint32_t  gpioe_moder = GPIOE->MODER;
        ENABLE_PWM(gpioe_moder,  9, en_a);
        ENABLE_PWM(gpioe_moder, 11, en_b);
        ENABLE_PWM(gpioe_moder, 13, en_c);

        GPIOE->MODER = gpioe_moder;
        TIM1->CCR1 = PWM_MAX_COUNT - pwm_a;
        TIM1->CCR2 = PWM_MAX_COUNT - pwm_b;
        TIM1->CCR3 = PWM_MAX_COUNT - pwm_c;
        break;
    }
    }
}


inline __attribute__((always_inline))
static void bldc_set_commutation(int id, int step, float u_pwm)
{
    // Convert voltage to PWM duty cycle
    //
    const float k = PWM_MAX_COUNT / bldc_state.u_bat;

    int p = clamp(
        (int)( (bldc_state.u_bat + u_pwm) / 2 * k ),
        PWM_MAX_COUNT * 0.05,  PWM_MAX_COUNT * 0.95
    );

    int n = PWM_MAX_COUNT - p;

    switch (step) {
    case 0: bldc_set_pwm(id,  0, 0, 0,  0, 0, 0); break;
    case 1: bldc_set_pwm(id,  p, 0, n,  1, 0, 1); break;
    case 2: bldc_set_pwm(id,  0, p, n,  0, 1, 1); break;
    case 3: bldc_set_pwm(id,  n, p, 0,  1, 1, 0); break;
    case 4: bldc_set_pwm(id,  n, 0, p,  1, 0, 1); break;
    case 5: bldc_set_pwm(id,  0, n, p,  0, 1, 1); break;
    case 6: bldc_set_pwm(id,  p, n, 0,  1, 1, 0); break;
    case 7: bldc_set_pwm(id,  0, 0, 0,  1, 1, 1); break;
    }
}


inline __attribute__((always_inline))
static void bldc_set_led(int id, int pwm_led)
{
    switch (id) {
    case ID_FL: TIM3->CCR4 = pwm_led;                   break;
    case ID_FR: TIM2->CCR4 = PWM_MAX_COUNT - pwm_led;   break;
    case ID_RL: TIM4->CCR4 = pwm_led;                   break;
    case ID_RR: TIM1->CCR4 = PWM_MAX_COUNT - pwm_led;   break;
    }
}


static void bldc_set_outputs(void)
{
    for (int id=0; id<4; id++) {
        const struct motor_state *m = &bldc_state.motors[id];

        if (bldc_state.errors)
            bldc_set_commutation(id, 0, m->u_pwm);
        else
            bldc_set_commutation(id, m->step, m->u_pwm);
    }

    for (int id=0; id<4; id++) {
        const struct motor_state *m = &bldc_state.motors[id];
        bldc_set_led(id, (gamma_tab[m->led] * PWM_MAX_COUNT) >> 16);
    }
}


void DMA2_Stream0_IRQHandler(void)
{
    uint16_t tim7_cnt = TIM7->CNT;

    bldc_get_measurements();
    bldc_irq_time1 = TIM7->CNT;

    bldc_irq_handler();
    bldc_irq_time2 = TIM7->CNT;

    bldc_set_outputs();
    bldc_irq_time3 = TIM7->CNT;

    DMA2->LIFCR = DMA_LIFCR_CTCIF0;
    DMA2->LIFCR;  // dummy read to prevent IRQ glitches

    bldc_irq_count++;

    bldc_irq_time3 = (uint16_t)(bldc_irq_time3 - bldc_irq_time2);
    bldc_irq_time2 = (uint16_t)(bldc_irq_time2 - bldc_irq_time1);
    bldc_irq_time1 = (uint16_t)(bldc_irq_time1 - tim7_cnt);
    bldc_irq_time  = (uint16_t)(TIM7->CNT - tim7_cnt);
}


/**
 * Initilialize BLDC control hardware
 *
 */
void bldc_driver_init(void)
{
    // Enable peripheral clocks
    //
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN |
                    RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_DMA2EN;

    RCC->APB1ENR |= RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4 | RCC_APB1Periph_TIM5;

    RCC->APB2ENR |= RCC_APB2Periph_TIM1 | RCC_APB2ENR_ADC1EN  | RCC_APB2ENR_ADC2EN  | RCC_APB2ENR_ADC3EN;

    DMA_DeInit(DMA2_Stream0);
    ADC_DeInit();
    TIM_DeInit(TIM1);
    TIM_DeInit(TIM2);
    TIM_DeInit(TIM3);
    TIM_DeInit(TIM4);
    TIM_DeInit(TIM5);

    // Set up PWM timers with phase shift
    //
    //  TIM3  FL
    //  TIM2  FR
    //  TIM4  RL
    //  TIM1  RR
    //
    TIM_TimeBaseInitTypeDef tb_init =  {
        .TIM_CounterMode = TIM_CounterMode_CenterAligned1,
        .TIM_Period = PWM_MAX_COUNT
    };

    tb_init.TIM_Prescaler = 0;
    TIM_TimeBaseInit(TIM3, &tb_init);
    TIM_TimeBaseInit(TIM2, &tb_init);
    TIM_TimeBaseInit(TIM4, &tb_init);
    tb_init.TIM_Prescaler = 1;
    TIM_TimeBaseInit(TIM1, &tb_init);


    TIM_OCInitTypeDef  oc_init = {
        .TIM_OCMode       = TIM_OCMode_PWM1,
        .TIM_OutputState  = TIM_OutputState_Enable,
        .TIM_Pulse        = 0
    };

#define TIM_OC1234Init(x, y)    \
        TIM_OC1Init(x, y);  TIM_OC2Init(x, y);  \
        TIM_OC3Init(x, y);  TIM_OC4Init(x, y);

    // FL 0°
    oc_init.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1234Init(TIM3, &oc_init);
    TIM_SetCounter(TIM3, PWM_MAX_COUNT * 0 / 2);

    // FR 90°
    oc_init.TIM_OCPolarity = TIM_OCPolarity_Low;
    TIM_OC1234Init(TIM2, &oc_init);
    TIM_SetCounter(TIM2, PWM_MAX_COUNT * 1 / 2);

    // RL 270°
    oc_init.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1234Init(TIM4, &oc_init);
    TIM_SetCounter(TIM4, PWM_MAX_COUNT * 1 / 2);

    // RR 180°
    oc_init.TIM_OCPolarity = TIM_OCPolarity_Low;
    TIM_OC1234Init(TIM1, &oc_init);
    TIM_SetCounter(TIM1, PWM_MAX_COUNT * 0 / 2);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);

    bldc_set_outputs();

    // Set up PWM output pins
    //
    GPIO_InitTypeDef  gpio_pwm = {
        .GPIO_Mode  = GPIO_Mode_AF,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_OType = GPIO_OType_PP,
        .GPIO_PuPd  = GPIO_PuPd_NOPULL
    };

    //  PC6     TIM3_CH1    FL_PWM_A
    //  PC8     TIM3_CH3    FL_PWM_C
    //  PC7     TIM3_CH2    FL_PWM_B
    //  PC9     TIM3_CH4    FL_LED
    //
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_TIM3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_TIM3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource8, GPIO_AF_TIM3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_TIM3);

    gpio_pwm.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_Init(GPIOC, &gpio_pwm);

    //  PA15    TIM2_CH1    FR_PWM_A
    //  PB3     TIM2_CH2    FR_PWM_B
    //  PB10    TIM2_CH3    FR_PWM_C
    //  PB11    TIM2_CH4    FR_LED
    //
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource15, GPIO_AF_TIM2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource3,  GPIO_AF_TIM2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_TIM2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_TIM2);

    gpio_pwm.GPIO_Pin = GPIO_Pin_15;
    GPIO_Init(GPIOA, &gpio_pwm);

    gpio_pwm.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_Init(GPIOB, &gpio_pwm);

    //  PD12    TIM4_CH1    RL_PWM_A
    //  PD13    TIM4_CH2    RL_PWM_B
    //  PD14    TIM4_CH3    RL_PWM_C
    //  PD15    TIM4_CH4    RL_LED
    //
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource12, GPIO_AF_TIM4);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource13, GPIO_AF_TIM4);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource14, GPIO_AF_TIM4);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource15, GPIO_AF_TIM4);

    gpio_pwm.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init(GPIOD, &gpio_pwm);

    //  PE9     TIM1_CH1    RR_PWM_A
    //  PE11    TIM1_CH2    RR_PWM_B
    //  PE13    TIM1_CH3    RR_PWM_C
    //  PE14    TIM1_CH4    RR_LED
    //
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource9,  GPIO_AF_TIM1);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource11, GPIO_AF_TIM1);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource13, GPIO_AF_TIM1);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource14, GPIO_AF_TIM1);

    gpio_pwm.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_11 | GPIO_Pin_13 | GPIO_Pin_14;
    GPIO_Init(GPIOE, &gpio_pwm);

    //  PE15    GPIO        THDN
    //
    GPIO_Init(GPIOE, &(GPIO_InitTypeDef) {
        .GPIO_Pin   = GPIO_Pin_15,
        .GPIO_Mode  = GPIO_Mode_IN,
        .GPIO_PuPd  = GPIO_PuPd_UP
    });

    // Use TIM5_CH3 rising edge to trigger the ADC
    //
    TIM_TimeBaseInit(TIM5, &(TIM_TimeBaseInitTypeDef) {
        .TIM_CounterMode  = TIM_CounterMode_Up,
        .TIM_Period       = TIMEBASE_FREQ / ADC_FREQ - 1
    });

    TIM_OCInitTypeDef  tim5_oc = {
        .TIM_OCMode       = TIM_OCMode_PWM1,
        .TIM_OCPolarity   = TIM_OCPolarity_High,
        .TIM_OutputState  = TIM_OutputState_Enable,
        .TIM_Pulse        = TIM5->ARR / 2
    };

    TIM_OC3Init(TIM5, &tim5_oc);

    // Initialize ADC input pins
    //
    //  PA0   ADC123_IN0    RR_EMF_A
    //  PA1   ADC123_IN1    RR_EMF_B
    //  PA2   ADC123_IN2    RL_EMF_C
    //  PA3   ADC123_IN3    FL_EMF_C
    //  PA6   ADC12_IN6     FL_EMF_B
    //  PA7   ADC12_IN7     FL_EMF_A
    //  PB0   ADC12_IN8     RL_EMF_A
    //  PB1   ADC12_IN9     ADC_AUX
    //  PC0   ADC123_IN10   FR_EMF_A
    //  PC1   ADC123_IN11   FR_EMF_B
    //  PC2   ADC123_IN12   FR_EMF_C
    //  PC3   ADC123_IN13   UM_SENSE
    //  PC4   ADC12_IN14    RR_EMF_C
    //  PC5   ADC12_IN15    RL_EMF_B
    //
    GPIO_InitTypeDef  gpio_adc = {
        .GPIO_Mode  = GPIO_Mode_AN,
        .GPIO_PuPd  = GPIO_PuPd_NOPULL
    };

    gpio_adc.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_6 | GPIO_Pin_3 |
                        GPIO_Pin_2 | GPIO_Pin_1 | GPIO_Pin_0;
    GPIO_Init(GPIOA, &gpio_adc);

    gpio_adc.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_0;
    GPIO_Init(GPIOB, &gpio_adc);

    gpio_adc.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 |
                        GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_Init(GPIOC, &gpio_adc);

    // Set up ADC DMA
    //
    DMA_Init(DMA2_Stream0, &(DMA_InitTypeDef) {
        .DMA_Channel            = DMA_Channel_0,
        .DMA_PeripheralBaseAddr = (uint32_t)&ADC->CDR,
        .DMA_Memory0BaseAddr    = (uint32_t)&dma_buf,
        .DMA_DIR                = DMA_DIR_PeripheralToMemory,
        .DMA_BufferSize         = ARRAY_SIZE(dma_buf),
        .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
        .DMA_MemoryInc          = DMA_MemoryInc_Enable,
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord,
        .DMA_MemoryDataSize     = DMA_MemoryDataSize_Word,
        .DMA_Mode               = DMA_Mode_Circular,
        .DMA_Priority           = DMA_Priority_VeryHigh,

        .DMA_FIFOMode           = DMA_FIFOMode_Enable,
        .DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,
        .DMA_MemoryBurst        = DMA_MemoryBurst_INC4,
        .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single
    });

    // Initialize DMA interrupt
    //
    NVIC_Init(&(NVIC_InitTypeDef) {
        .NVIC_IRQChannel = DMA2_Stream0_IRQn,
        .NVIC_IRQChannelPreemptionPriority = 1,
        .NVIC_IRQChannelCmd = ENABLE
    });

    DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, ENABLE);

    DMA_Cmd(DMA2_Stream0, ENABLE);

    // Initialize ADC converters
    // F_ADC = APB2 / 4 = 21 MHz
    //
    ADC_CommonInit(&(ADC_CommonInitTypeDef) {
        .ADC_Mode = ADC_TripleMode_RegSimult_InjecSimult,
        .ADC_Prescaler = ADC_Prescaler_Div4,
        .ADC_DMAAccessMode = ADC_DMAAccessMode_1
    });

    ADC_InitTypeDef adc_init = {
        .ADC_Resolution = ADC_Resolution_12b,
        .ADC_ScanConvMode = ENABLE,
        .ADC_ContinuousConvMode = DISABLE,
        .ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising,
        .ADC_ExternalTrigConv = ADC_ExternalTrigConv_T5_CC3,
        .ADC_DataAlign = ADC_DataAlign_Right,
        .ADC_NbrOfConversion = 4
    };

    ADC_Init(ADC1, &adc_init);
    adc_init.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_Init(ADC2, &adc_init);
    ADC_Init(ADC3, &adc_init);

    // See AN2834 - How to get the best ADC accuracy
    //
    // R_AIN_max = (K - 0.5) / (F_ADC * C_ADC * ln(2^(N/2)))
    // ->  (3 - 0.5) / (21e6 Hz  * 4 pF  * ln(2^(12/2))) ~= 7156 Ohm
    //
    ADC_RegularChannelConfig(ADC1, ADC_Channel_7,  1, ADC_SampleTime_3Cycles);  // FL_EMF_A
    ADC_RegularChannelConfig(ADC2, ADC_Channel_6,  1, ADC_SampleTime_3Cycles);  // FL_EMF_B
    ADC_RegularChannelConfig(ADC3, ADC_Channel_3,  1, ADC_SampleTime_3Cycles);  // FL_EMF_C

    ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 2, ADC_SampleTime_3Cycles);  // FR_EMF_A
    ADC_RegularChannelConfig(ADC2, ADC_Channel_11, 2, ADC_SampleTime_3Cycles);  // FR_EMF_B
    ADC_RegularChannelConfig(ADC3, ADC_Channel_12, 2, ADC_SampleTime_3Cycles);  // FR_EMF_C

    ADC_RegularChannelConfig(ADC1, ADC_Channel_14, 3, ADC_SampleTime_3Cycles);  // RR_EMF_C (!)
    ADC_RegularChannelConfig(ADC2, ADC_Channel_1,  3, ADC_SampleTime_3Cycles);  // RR_EMF_B
    ADC_RegularChannelConfig(ADC3, ADC_Channel_0,  3, ADC_SampleTime_3Cycles);  // RR_EMF_A (!)

    ADC_RegularChannelConfig(ADC1, ADC_Channel_8,  4, ADC_SampleTime_3Cycles);  // RL_EMF_A
    ADC_RegularChannelConfig(ADC2, ADC_Channel_15, 4, ADC_SampleTime_3Cycles);  // RL_EMF_B
    ADC_RegularChannelConfig(ADC3, ADC_Channel_2,  4, ADC_SampleTime_3Cycles);  // RL_EMF_C

    // Set up injected channels
    //
    ADC_InjectedSequencerLengthConfig(ADC1, 1);
    ADC_InjectedSequencerLengthConfig(ADC2, 1);
    ADC_InjectedChannelConfig(ADC1, ADC_Channel_13, 1, ADC_SampleTime_3Cycles); // UM_SENSE
    ADC_InjectedChannelConfig(ADC2, ADC_Channel_9 , 1, ADC_SampleTime_3Cycles); // ADC_AUX
    ADC_AutoInjectedConvCmd(ADC1, ENABLE);
    ADC_AutoInjectedConvCmd(ADC2, ENABLE);

    // Enable ADCs
    //
    ADC_MultiModeDMARequestAfterLastTransferCmd(ENABLE);
    ADC_Cmd(ADC1, ENABLE);
    ADC_Cmd(ADC2, ENABLE);
    ADC_Cmd(ADC3, ENABLE);

    // Start PWM timers synchronously
    // TIM4 is the only timer that can act as a master for TIM 1, 2, 3 and 5
    //
    TIM_SelectOutputTrigger(TIM4, TIM_TRGOSource_Enable);
    TIM_SelectMasterSlaveMode(TIM4, TIM_MasterSlaveMode_Enable);

    TIM_SelectInputTrigger(TIM1, TIM_TS_ITR3);
    TIM_SelectSlaveMode(TIM1, TIM_SlaveMode_Trigger);

    TIM_SelectInputTrigger(TIM2, TIM_TS_ITR3);
    TIM_SelectSlaveMode(TIM2, TIM_SlaveMode_Trigger);

    TIM_SelectInputTrigger(TIM3, TIM_TS_ITR3);
    TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Trigger);

    TIM_SelectInputTrigger(TIM5, TIM_TS_ITR2);
    TIM_SelectSlaveMode(TIM5, TIM_SlaveMode_Trigger);

    // Compensate master/slave delay for TIM1
    // (verified by measurement)
    //
    TIM2->CNT += 2;
    TIM3->CNT += 2;
    TIM4->CNT += 2;
    TIM5->CNT += 2;

    TIM_Cmd(TIM4, ENABLE);
}


// -------------------- Shell commands --------------------
//
#include "command.h"
#include <stdlib.h>
#include <stdio.h>


static void cmd_bldc_show(int argc, char *argv[])
{
    const char *id_str[] = { "FL", "FR", "RL", "RR" };

    for (int id=0; id<4; id++) {
        const struct motor_state *m = &bldc_state.motors[id];
        printf("%s  : PWM %6.3f V, %6.3f RPM, step %d, pos %d\n"
               "      ADC %6.3f %6.3f %6.3f V\n\n",
            id_str[id], m->u_pwm,
            m->rpm_filter.y[0], m->step, m->pos,
            m->u_a, m->u_b, m->u_c
        );
    }

    printf("u_bat: %6.3f V\n",  bldc_state.u_bat);
    printf("u_aux: %6.3f V\n",  bldc_state.u_aux);
    printf("THDN : %d\n\n",     bldc_state.thdn);

    printf("irq_count = %lu\n",    bldc_irq_count);
    printf("irq_time  = %2lu %2lu %2lu = %2lu us\n",
            bldc_irq_time1, bldc_irq_time2, bldc_irq_time3,
            bldc_irq_time
    );
}


static void cmd_set_pwm(int argc, char *argv[])
{
    if (argc != 3)
        goto usage;

    int id = atoi(argv[1]);
    if (id < 0 || id > 3)
        goto usage;

    int step = atoi(argv[2]);
	if (id < 0 || id > 7)
	    goto usage;
	
	bldc_state.motors[id].step = step;
    bldc_state.motors[id].u_pwm = atof(argv[2]);
    return;

usage:
    printf("usage: %s <id> <step> <u_pwm>\n", argv[0]);
}


SHELL_CMD(bldc_show,  (cmdfunc_t)cmd_bldc_show, "Show BLDC state")
SHELL_CMD(set_pwm,    (cmdfunc_t)cmd_set_pwm,   "Set PWM output")
