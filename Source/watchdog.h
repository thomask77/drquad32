#pragma once

#define WATCHDOG_TIMEOUT    10  // [ms]

void watchdog_set_timeout(int ms);
void watchdog_task(void *pvParameters);
