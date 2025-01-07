#pragma once

#include <core/assert.h>

#define log_error(fmt, ...) \
    printk("ERROR: %s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)

#ifdef DEBUG_BUILD
#define debug(fmt, ...) printk("DEBUG: %s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define debug(fmt, ...) // No-op in release builds
#endif

#define debugloc() ({                                               \
    debug("cpu[%d ncli: %d] tid[%d:%d] ret[%p]\n",                  \
          getcpuid(), cpu->ncli, gettid(), getpid(), __retaddr(0)); \
})
