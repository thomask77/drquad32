#include "stm32f4xx.h"
#include "term_xbee.h"
#include "term_usb.h"
#include "fault_handler.h"
#include "watchdog.h"
#include "version.h"
#include "board.h"
#include "shell_task.h"
#include "led_task.h"
#include "syscalls.h"
#include "FreeRTOS.h"
#include "task.h"
#include "parameter.h"
#include "bldc_driver.h"
#include "bldc_task.h"
#include "flight_ctrl.h"
#include "debug_dac.h"
#include "i2c_driver.h"
#include "sensors.h"
#include "dma_io_driver.h"
#include "rc_input.h"
#include "gpn_foo.h"
#include <stdio.h>
#include <unistd.h>

#include "matrix3f.h"

static TaskHandle_t wdog_handle;
static TaskHandle_t sensor_handle;
static TaskHandle_t led_handle;
static TaskHandle_t shell_handle;
static TaskHandle_t usb_handle;
static TaskHandle_t bldc_handle;
static TaskHandle_t flight_handle;


static void init_task(void *pvParameters)
{
    board_init();
    board_set_leds(LED_RED);

    xbee_init();

    printf("\n");
    print_version_info(&version_info, 0);

    print_fault_info();

    param_set_defaults();
    param_load();

    debug_dac_init();
    i2c_init();
    dma_io_init();
    rc_init();

    // The watchdog task is created with idle priority to check
    // for CPU starvation as a side effect.
    //
    printf("Starting watchdog task..\n");
    xTaskCreate(watchdog_task, "watchdog", 256, NULL, 0, &wdog_handle);
    vTaskDelay(100);

    printf("Starting LED task..\n");
    xTaskCreate(led_task, "LEDs", 256, NULL, 0, &led_handle);
    vTaskDelay(100);

    printf("Starting sensor task..\n");
    xTaskCreate(sensor_task, "sensors", 1024, NULL, 3, &sensor_handle);
    vTaskDelay(100);

    printf("Starting motor control task..\n");
    xTaskCreate(bldc_task, "bldc", 1024, NULL, 4, &bldc_handle);
    vTaskDelay(100);

    printf("Starting flight control task..\n");
    xTaskCreate(flight_ctrl, "flight_ctrl", 1024, NULL, 1, &flight_handle);
    vTaskDelay(100);

    printf("Starting USB shell task..\n");
    term_usb_init();


    // passed by reference. must be static.
    static struct shell_params shell_params = {
        .fd_stdin  = FD_DEV_USB | STDIN_FILENO,
        .fd_stdout = FD_DEV_USB | STDOUT_FILENO,
        .fd_stderr = FD_DEV_USB | STDERR_FILENO
    };

    xTaskCreate(shell_task, "usb_shell", 1024, &shell_params, 0, &usb_handle);
    vTaskDelay(100);

    printf("Starting serial shell task..\n");
    xTaskCreate(shell_task, "uart_shell", 1024, NULL, 0, &shell_handle);
    vTaskDelay(100);

//    printf("Starting GPN special blinky foo..\n");
//    xTaskCreate(gpn_foo_task, "gpn_foo", 1024, NULL, 1, NULL);
//    vTaskDelay(100);

    while (!stdin_chars_avail())
        send_imu_data();

    // We're not needed any more.
    //
    vTaskDelete(NULL);
}


int main(void)
{
    // Relocate interrupt vectors
    //
    extern void *g_pfnVectors;
    SCB->VTOR = (uint32_t)&g_pfnVectors;

    // Set up NVIC priorities for FreeRTOS
    //
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    // Create init task and start the scheduler
    //
    xTaskCreate(init_task, "init", 1024, NULL, tskIDLE_PRIORITY, NULL);
    vTaskStartScheduler();
}

