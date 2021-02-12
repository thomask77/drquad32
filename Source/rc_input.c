#include "rc_input.h"
#include "rc_ppm.h"
#include "rc_dsm2.h"
#include "rc_sbus.h"
#include <string.h>
#include <stdio.h>

struct rc_config rc_config = {
    .mode = RC_MODE_PPM,
    .expected_channels = 8
};


static const char *rc_strmode(int mode)
{
    switch (rc_config.mode) {
    case RC_MODE_NONE:  return "NONE";
    case RC_MODE_PPM:   return "PPM";
    case RC_MODE_SERVO: return "SERVO";
    case RC_MODE_DSM2:  return "DSM2";
    case RC_MODE_SBUS:  return "SBUS";
    default:            return "Unknown";
    }
}


void rc_update(struct rc_input *rc)
{
    switch (rc_config.mode) {
    case RC_MODE_PPM:   rc_ppm_update(rc);  break;
    case RC_MODE_DSM2:  rc_dsm2_update(rc); break;
    case RC_MODE_SBUS:  rc_sbus_update(rc); break;
//     case RC_MODE_SERVO: rc_dma_update(rc);  break;  //TODO
    case RC_MODE_NONE:
    default:
        memset(rc, 0, sizeof(*rc));
        break;
    }
}


void rc_init(void)
{
    printf("Initializing %s remote control..\n",
        rc_strmode(rc_config.mode)
    );

    switch (rc_config.mode) {
    case RC_MODE_NONE:  break;
    case RC_MODE_PPM:   rc_ppm_init();  break;
    case RC_MODE_SERVO: /* TODO */      break;
    case RC_MODE_DSM2:  rc_dsm2_init(); break;
    case RC_MODE_SBUS:  rc_sbus_init(); break;
    default: break;
    }
}


// -------------------- Shell commands --------------------
//
#include "command.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>

#include "bldc_task.h"

static void cmd_rc_show(void)
{
    struct rc_input rc;

    rc_update(&rc);

    printf(
        "mode=%s, valid=%d, rssi=%3d, num_channels=%d\n",
        rc_strmode(rc_config.mode), rc.valid, rc.rssi, rc.num_channels
    );

    for (unsigned int i=0; i<RC_MAX_CHANNELS; i++) {
        char bar[31];
        strnbar(bar, sizeof(bar), rc.channels[i], 1000, 2000);

        printf("%4d: [%s] %4d us\n", i, bar, rc.channels[i]);
    }
}


SHELL_CMD(rc_show, (cmdfunc_t)cmd_rc_show, "Show remote control")

