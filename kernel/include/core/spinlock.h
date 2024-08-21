#pragma once

#include <core/types.h>

typedef struct spinlock_t {
    u8      lock;   // actual lock.
    u8      guard;  // guard lock.
    u8      cpuid;  // cpu ID of the core that acquired the lock.
    tid_t   tid;    // thread ID of thread that acquired the lock.

    // DEBUG purposes only //
    i16     line;   // line at which spinlock was held.
    char    *file;  // file in which spinlock was acquired.
    char    *func;  // function that acquired the spinlock.
} spinlock_t;

#define spin_assert(s)          ({ ;})
#define spin_lock(s)            ({ spin_assert(s); })
#define spin_unlock(s)          ({ spin_assert_locked(s); })
#define spin_islocked(s)        ({ spin_assert(s); })
#define spin_assert_locked(s)   ({ spin_assert(s); })