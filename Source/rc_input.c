#include "rc_input.h"
#include "rc_ppm_sum.h"
#include "rc_separate.h"
// #include "rc_dsm2.h"
// #include "rc_sbus.h"
#include <string.h>
#include <stdio.h>


struct rc_config rc_config;
struct rc_input  rc_input;


static const char *rc_driver_to_str(enum rc_driver driver)
{
    switch (driver) {
    case RC_DRIVER_NONE:            return "NONE";
    case RC_DRIVER_PPM_SUM:         return "PPM_SUM";
    case RC_DRIVER_PPM_SEPARATE:    return "SEPARATE";
    // case RC_DRIVER_DSM2:            return "DSM2";
    // case RC_DRIVER_SBUS:            return "SBUS";
    default:                        return "Invalid";
    }
}


static const char *rc_function_to_str(enum rc_function function)
{
    switch (function) {
    case RC_FUNCTION_NONE:      return "NONE";
    case RC_FUNCTION_PITCH:     return "PITCH";
    case RC_FUNCTION_ROLL:      return "ROLL";
    case RC_FUNCTION_THRUST:    return "THRUST";
    case RC_FUNCTION_YAW:       return "YAW";
    case RC_FUNCTION_FMOD:      return "FMOD";
    case RC_FUNCTION_HOLD:      return "HOLD";
    case RC_FUNCTION_AUX1:      return "AUX1";
    case RC_FUNCTION_AUX2:      return "AUX2";
    default:                    return "Invalid";
    }
}


static void update_channel(struct rc_channel *ch)
{
    // Center pulse width range around 0
    //
    float v = ch->pulse - ch->center;

    // Apply deadband and adjust slope
    //
    v = deadband(v, ch->deadband) * (ch->max - ch->center)
            / (ch->max - ch->center - ch->deadband);

    // Normalize to -1 .. +1
    //
    v = clamp(v / (ch->max - ch->center), -1, 1);

    if (ch->invert)
        v = -v;

    ch->value = v;

    // Save mapped value
    //
    if (ch->function != 0) {
        assert(ch->function >= 0 && ch->function < ARRAY_SIZE(rc_input.values));
        rc_input.values[ch->function] = ch->value;
    }
}


void rc_input_update()
{
    switch (rc_config.driver) {
    case RC_DRIVER_PPM_SUM:         rc_ppm_sum_update();        break;
    case RC_DRIVER_PPM_SEPARATE:    rc_ppm_separate_update();   break;
    // case RC_DRIVER_DSM2:            rc_dsm2_update();           break;
    // case RC_DRIVER_SBUS:            rc_sbus_update();           break;
    default:
        break;
    }

    for (int i=0; i<ARRAY_SIZE(rc_input.channels); i++)
        update_channel(&rc_input.channels[i]);
}


void rc_input_init(void)
{
    printf("Initializing RC driver %s..\n",  rc_driver_to_str(rc_config.driver));

    switch (rc_config.driver) {
    case RC_DRIVER_PPM_SUM:         rc_ppm_sum_init();      break;
    case RC_DRIVER_PPM_SEPARATE:    rc_ppm_separate_init(); break;
    // case RC_DRIVER_DSM2:            rc_dsm2_init();         break;
    // case RC_DRIVER_SBUS:            rc_sbus_init();         break;

    default:
        break;
    }
}


// -------------------- Shell commands --------------------
//
#include "command.h"
#include "util.h"
#include "ansi.h"
#include "FreeRTOS.h"
#include "task.h"
#include "syscalls.h"
#include <stdlib.h>


static void cmd_rc_show(void)
{
    printf(ANSI_CLEAR ANSI_CURSOR_OFF);

    while (!stdin_chars_avail()) {
        printf(ANSI_HOME);

        printf(
            "driver=%8s, valid=%d, rssi=%3d\n",
            rc_driver_to_str(rc_config.driver),
            rc_input.valid, rc_input.rssi
        );

        for (int i=0; i<ARRAY_SIZE(rc_input.channels); i++) {
            struct rc_channel *ch = &rc_input.channels[i];
            char bar[31];
            strnbar(bar, sizeof(bar), ch->value, -1, 1);

            printf("CH%d: %4dus [%s] %6.3f", i, ch->pulse, bar, ch->value);

            if (ch->function != RC_FUNCTION_NONE)
                printf(" -> %s", rc_function_to_str(ch->function));

            printf("\n");
        }

        vTaskDelay(100);
    }

    printf(ANSI_CURSOR_ON);
}


SHELL_CMD(rc_show, (cmdfunc_t)cmd_rc_show, "Show remote control")
