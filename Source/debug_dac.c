#include "debug_dac.h"
#include "parameter.h"
#include "util.h"
#include "stm32f4xx.h"


static float  dummy_null;

static struct param_cache {
    int   id;
    int   type;
    void  *ptr;
} dac1_cache, dac2_cache;

struct dac_config  dac_config;


static inline float get_value(struct param_cache *c, int id)
{
    if (c->id != id) {
        const struct param_info *p;
        if ((p = param_get_info(id))) {
            c->id   = p->id;
            c->type = p->type;
            c->ptr  = p->flt.ptr;
        }
        else {
            c->id   = id;
            c->type = PTYPE_FLOAT;
            c->ptr  = &dummy_null;
        }
    }

    if (c->type == PTYPE_FLOAT)
        return *(float*)c->ptr;
    else
        return *(int*)c->ptr;
}


void debug_dac_update(void)
{
    struct dac_config *c = &dac_config;
    float f1 = (get_value(&dac1_cache, c->dac1_id) + c->dac1_offset) / c->dac1_scale;
    float f2 = (get_value(&dac2_cache, c->dac2_id) + c->dac2_offset) / c->dac2_scale;

    DAC->DHR12R1 = clamp(f1 * (4096 / 3.3) + 2048, 0, 4095);
    DAC->DHR12R2 = clamp(f2 * (4096 / 3.3) + 2048, 0, 4095);
}


void debug_dac_init(void)
{
    // Enable peripheral clocks
    //
    RCC->APB1ENR |= RCC_APB1Periph_DAC;
    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOA;

    // The output buffer introduces some distortion and clipping
    //
    DAC_Init(DAC_Channel_1, &(DAC_InitTypeDef) {
        .DAC_OutputBuffer = DAC_OutputBuffer_Enable
    });

    DAC_Init(DAC_Channel_2, &(DAC_InitTypeDef) {
        .DAC_OutputBuffer = DAC_OutputBuffer_Enable
    });

    DAC_Cmd(DAC_Channel_1, ENABLE);
    DAC_Cmd(DAC_Channel_2, ENABLE);

    // PA4  DAC1
    // PA5  DAC2
    //
    GPIO_Init(GPIOA, &(GPIO_InitTypeDef) {
        .GPIO_Pin  = GPIO_Pin_4 | GPIO_Pin_5,
        .GPIO_Mode = GPIO_Mode_AN
    });

    dac1_cache.id = -1;
    dac2_cache.id = -1;
}

