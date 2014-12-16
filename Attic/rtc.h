#ifndef RTC_H

#include <stdint.h>
#include <sys/time.h>

#define RTC_PRESCALER         (12000000/128)
#define RTC_JIFFIES_TO_US(x)  ((x)*128 / 12)

extern int      _gettimeofday(struct timeval *tp, void *tzvp);
extern int      _settimeofday(const struct timeval *tp, const struct timezone *tzvp);

extern uint32_t rtc_get_jiffies(void);
extern void     rtc_init(void);

#endif
