#pragma once

#include <sys/system.h>
#include <sync/atomic.h>
#include <sync/preempt.h>
#include <lib/printk.h>
#include <lib/types.h>

struct thread;
extern tid_t thread_self(void);
extern tid_t thread_gettid(thread_t *);
typedef struct spinlock
{
    int line;
    char *file;
    void *retaddr;
    cpu_t *processor;
    thread_t *thread;
    atomic_t lock; // is spinlock acquired?
} spinlock_t;

#define SPINLOCK_INIT() ((spinlock_t){ \
    .line = 0,                         \
    .lock = 0,                         \
    .file = NULL,                      \
    .thread = NULL,                    \
    .retaddr = NULL,                   \
    .processor = NULL,                 \
})

// assert_msg lock pointer
#define spin_assert_msg(lk, __msg__) ({ assert_msg((lk), __msg__); })

// assert_msg lock pointer
#define spin_assert(lk) ({ spin_assert_msg((lk), "no spinlock!"); })

// assert_msg spinlock acquisition
#define spin_assert_locked(lk) ({                                               \
    assert_msg(spin_locked((lk)),                                               \
               "%s:%d: CPU%d: current: %d: retaddr: %lX: spinlock not held!\n", \
               __FILE__, __LINE__, cpu_id, thread_self(), __retaddr(0));        \
})

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
    assert_msg(!spin_locked(lk), "%s:%d: \
CPU%d thread[%d], status: %d: current[%d]: \
spinlock held at %s:%d, retaddr: %p!",                  \
               __FILE__, __LINE__,                      \
               cpu_id, thread_gettid((lk)->thread),     \
               atomic_read(&(lk)->lock), thread_self(), \
               (lk)->file, (lk)->line, (lk)->retaddr);  \
    barrier();                                          \
    while (atomic_xchg(&(lk)->lock, 1))                 \
    {                                                   \
        popcli();                                       \
        cpu_pause();                                    \
        pushcli();                                      \
    }                                                   \
    (lk)->line = __LINE__;                              \
    (lk)->file = __FILE__;                              \
    (lk)->processor = cpu;                              \
    (lk)->thread = current;                             \
    (lk)->retaddr = __retaddr(0);                       \
})

// release spinlock
#define spin_unlock(lk) ({       \
    spin_assert(lk);             \
    spin_assert_locked(lk);      \
    (lk)->thread = NULL;         \
    (lk)->processor = NULL;      \
    (lk)->line = 0;              \
    (lk)->retaddr = 0;           \
    (lk)->file = NULL;           \
    barrier();                   \
    atomic_xchg(&(lk)->lock, 0); \
    popcli();                    \
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
