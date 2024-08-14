#pragma once

#include <ginger/types.h>

typedef unsigned int  jiffies32_t;
typedef unsigned long jiffies64_t;

#if defined (__x86_64__)
typedef jiffies64_t     jiffies_t;
#elif (__x86__)
typedef jiffies32_t     jiffies_t;
#endif

extern void jiffies_update(void);
extern jiffies_t jiffies_get(void);
extern void jiffies_wait(jiffies_t jiffies);
extern void jiffies_wait_nosleep(jiffies_t jiffies);