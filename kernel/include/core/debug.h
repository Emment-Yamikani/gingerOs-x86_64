#pragma once

#include <lib/printk.h>

#define log_error(fmt, ...) \
    printk("ERROR: %s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)

#ifdef DEBUG_BUILD
#define debug(fmt, ...) printk("DEBUG: %s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define debug(fmt, ...) // No-op in release builds
#endif
