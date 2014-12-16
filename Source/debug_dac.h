#pragma once

struct dac_config {
    int   dac1_id;
    float dac1_scale;
    float dac1_offset;

    int   dac2_id;
    float dac2_scale;
    float dac2_offset;
};

extern struct dac_config dac_config;

void debug_dac_update(void);
void debug_dac_init(void);
