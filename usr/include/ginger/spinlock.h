#pragma once

#include "atomic.h"
#include "syscall.h"
#include "../types.h"

typedef struct {
    uint8_t     guard;
    uint8_t     locked;
    tid_t       thread;
} spinlock_t;

#define SPINLOCK_INIT() ((spinlock_t){ \
    .guard = 0,                        \
    .locked = 0,                       \
    .thread = 0,                       \
})

#define SPINLOCK_NEW()  ((spinlock_t *){&SPINLOCK_INIT()})

#define spin_islocked(lk) ({                           \
    while (atomic_test_and_set(&(lk)->guard))          \
        asm __volatile__("pause");                     \
    int locked = ((lk)->locked &&                      \
                  ((lk)->thread == sys_thread_self())) \
                     ? 1                               \
                     : 0;                              \
    atomic_clear(&(lk)->guard);                        \
    locked;                                            \
})

#define spin_assert_locked(lk) ({                  \
    assert(spin_islocked(lk), "lock is not held"); \
})

#define spin_lock(lk) ({                               \
    for (;;) {                                         \
        while (atomic_test_and_set(&(lk)->guard))      \
            asm __volatile__("pause");                 \
        assert(!(((lk)->locked) &&                     \
                 ((lk)->thread == sys_thread_self())), \
               "lock already acquired");               \
        if ((lk)->locked)                              \
            atomic_clear(&(lk)->guard);                \
        else                                           \
            break;                                     \
    }                                                  \
    (lk)->locked = 1;                                  \
    (lk)->thread = sys_thread_self();                  \
    atomic_clear(&(lk)->guard);                        \
})

#define spin_unlock(lk) ({                        \
    while (atomic_test_and_set(&(lk)->guard))     \
        asm __volatile__("pause");                \
    assert((((lk)->locked) &&                     \
            ((lk)->thread == sys_thread_self())), \
           "lock not acquired");                  \
    (lk)->locked = 0;                             \
    (lk)->thread = 0;                             \
    atomic_clear(&(lk)->guard);                   \
})
