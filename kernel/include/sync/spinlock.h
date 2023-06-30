#pragma once

#include <sys/system.h>
#include <sync/atomic.h>
#include <sync/preempt.h>
#include <lib/printk.h>
#include <lib/types.h>

struct thread;
typedef struct spinlock
{
    atomic_t lock; // is spinlock acquired?
    cpu_t *processor;
    thread_t *thread;
} spinlock_t;

#define SPINLOCK_INIT() ((spinlock_t){ \
    .lock = 0,                         \
    .thread = NULL,                    \
    .processor = NULL,                 \
})

// assert_msg lock pointer
#define spin_assert_msg(lk, __msg__) ({ assert_msg((lk), __msg__); })

// assert_msg lock pointer
#define spin_assert(lk) ({ spin_assert_msg((lk), "no spinlock!"); })

#define spin_locked(lk) ({                            \
    spin_assert(lk);                                  \
    pushcli();                                        \
    int held = atomic_read(&(lk)->lock);              \
    held = ((lk)->thread ? (lk)->thread == current    \
                         : (lk)->processor == cpu) && \
           held;                                      \
    popcli();                                         \
    held;                                             \
})

// acquire spinlock
#define spin_lock(lk) ({                                \
    spin_assert(lk);                                    \
    pushcli();                                          \
    assert_msg(!spin_locked(lk), "%s:%d: CPU%d thread: %p, \
status: %d: spinlock held!",                            \
               __FILE__, __LINE__, cpu_id,              \
               (lk)->thread, atomic_read(&(lk)->lock)); \
    barrier();                                          \
    while (atomic_xchg(&(lk)->lock, 1))                 \
    {                                                   \
        popcli();                                       \
        pause();                                        \
        pushcli();                                      \
    }                                                   \
    (lk)->processor = cpu;                              \
    (lk)->thread = current;                             \
})

// release spinlock
#define spin_unlock(lk) ({                                      \
    spin_assert(lk);                                            \
    assert_msg(spin_locked(lk), "%s:%d: cpu%d: spinlock held!", \
               __FILE__, __LINE__, cpu_id);                     \
    (lk)->thread = NULL;                                        \
    (lk)->processor = NULL;                                     \
    barrier();                                                  \
    atomic_xchg(&(lk)->lock, 0);                                \
    popcli();                                                   \
})

/**
 * @brief try to acquire a spinlock
 *
 * @param lk
 * @return int '1' if acquisition was successful(lock wasn't held before) and '0' if acquisition failed(lock is already held)
 */
#define spin_trylock(lk) ({                   \
    assert_msg(lk, "no spinlock");            \
    pushcli();                                \
    barrier();                                \
    long held = !atomic_xchg(&(lk)->lock, 1); \
    if (held)                                 \
    {                                         \
        (lk)->processor = cpu;                \
        (lk)->thread = current;               \
    }                                         \
    else                                      \
        popcli();                             \
    held;                                     \
})

// assert_msg spinlock acquisition
#define spin_assert_locked(lk) ({ assert_msg(spin_locked((lk)),                                        \
    "%s:%d: CPU%d: spinlock not held!\n", __FILE__, __LINE__, \
    cpu_id); })
