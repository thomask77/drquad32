#include "led_task.h"
#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdbool.h>

/*
static int get_pwm(void)
{
    uint32_t tick  = xTaskGetTickCount();
    uint32_t level = (tick >> 4) & 31;
    if (level >= 16)
        level = 32-level;
    return (tick & 15) < level;
}
*/

static void led_update(void)
{
/*
    bool red, green;
    bool slow = !!(xTaskGetTickCount() & 512);
    bool fast = !!(xTaskGetTickCount() & 128);
    bool pwm  = get_pwm();

    switch(SystemStateMachine::GetState()) {
    case INIT_STATE:        red = fast;     green = !fast;  break;
    case SELF_TEST_STATE:   red = false;    green = fast;   break;
    case INPUT_POWER_LOST:  red = false;    green = fast;   break;
    case PROGRAMMING_STATE: red = true;     green = true;   break;
    case FATAL_ERROR_STATE: red = fast;     green = true;   break;
    case STANDBY_STATE:     red = false;    green = slow;   break;
    case READY_STATE:       red = false;    green = slow;   break;
    case ACTIVE_STATE:      red = false;    green = fast;   break;
    case ERROR_STATE:       red = slow;     green = true;   break;
    case ACTIVATE_STATE:    red = false;    green = fast;   break;
    case SHUT_DOWN_STATE:   red = fast;     green = fast;   break;
    default:                red = false;    green = false;  break;
    }

    red   = false;
    green = pwm;

    board_set_leds((red ? LED_RED : 0) | (green ? LED_GREEN : 0));
*/
}


void led_task(void *param)
{
    uint32_t t0 = xTaskGetTickCount();
    for (;;) {
        led_update();
        vTaskDelayUntil(&t0, 1);
    }
}
