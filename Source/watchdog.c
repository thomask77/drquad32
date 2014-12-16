#include "watchdog.h"
#include "stm32f4xx.h"
#include "board.h"
#include "FreeRTOS.h"
#include "task.h"


void watchdog_set_timeout(int ms)
{
    vPortEnterCritical();

    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    // The 32kHz LSI oscillator is quite inaccurate and has
    // a specified frequency range of 17 .. 47 kHz. We're
    // using the worst case value here:
    //
    IWDG_SetPrescaler(IWDG_Prescaler_32);
    IWDG_SetReload(ms * 47 / 32);

    IWDG_WriteAccessCmd(IWDG_WriteAccess_Disable);

    IWDG_Enable();
    IWDG_ReloadCounter();

    vPortExitCritical();
}


/**
 * System watchdog.
 *
 * \todo  check for running tasks, memory, motor control data, etc.
 *
 */
void watchdog_task(void *pvParameters)
{
    watchdog_set_timeout(WATCHDOG_TIMEOUT);

    TickType_t xLastWakeTime = xTaskGetTickCount();

    for(;;) {
        // Reload watchdog
        //
        IWDG_ReloadCounter();
        vTaskDelayUntil(&xLastWakeTime, WATCHDOG_TIMEOUT / 2);
    }
}
