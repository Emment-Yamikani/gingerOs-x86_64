#pragma once

#if defined (__x86_64__)
#include <arch/x86_64/thread.h>
#endif


typedef struct arch_thread_t {
    x86_64_thread_t thread; // arch-spec thread data.
    thread_t        *owner; // owner thread.
} arch_thread_t;