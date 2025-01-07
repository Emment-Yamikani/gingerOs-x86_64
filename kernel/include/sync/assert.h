#pragma once

#include <lib/printk.h>

#define assert(condition, fmt, ...) ({                                                  \
    if ((condition) == 0)                                                               \
        panic("PANIC: %s(): %s:%d: " fmt, __func__, __FILE__, __LINE__, ##__VA_ARGS__); \
})

#define assert_eq(v0, v1, fmt, ...) ({                                                       \
    if (((v0) == (v1)) == 0)                                                                 \
        panic("PANIC(#NE): %s(): %s:%d: " fmt, __func__, __FILE__, __LINE__, ##__VA_ARGS__); \
})

#define assert_ne(v0, v1, fmt, ...) ({                                                       \
    if ((v0) == (v1))                                                                        \
        panic("PANIC(#EQ): %s(): %s:%d: " fmt, __func__, __FILE__, __LINE__, ##__VA_ARGS__); \
})
