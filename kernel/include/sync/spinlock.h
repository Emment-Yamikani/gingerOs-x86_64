#pragma once

#include <sys/system.h>
#include <sync/atomic.h>
#include <sync/preempt.h>
#include <lib/printk.h>
#include <lib/types.h>
#include <lib/stdint.h>

extern tid_t thread_self(void);

typedef struct __spinlock_t {
    uint8_t s_guard;  // protects(guards) this structure.
    tid_t s_tid;      // thread ID of currently holding thread.
    uint8_t s_lock;   // A 1 in this bit position signifies that the lock is held.
    uint8_t s_apicid; // APIC ID of currently holding CPU.

    // for debugging.

    char *s_file; // source file in which this lock was held.
    int s_line;   // line at which this lock is held.
} spinlock_t;

#define SPINLOCK_INIT() ((spinlock_t){ \
    .s_guard = 0,                      \
    .s_lock = 0,                       \
    .s_tid = 0,                        \
    .s_line = 0,                       \
    .s_apicid = -1,                    \
    .s_file = NULL,                    \
})

#define SPINLOCK_NEW() (&SPINLOCK_INIT())
#define SPINLOCK(name) spinlock_t *name = SPINLOCK_NEW()

#define spin_assert(lk) ({ assert(lk, "No spinlock"); })

#define spin_islocked(lk) ({                                                                \
    spin_assert(lk);                                                                        \
    pushcli();                                                                              \
    while (atomic_test_and_set(&(lk)->s_guard))                                             \
    {                                                                                       \
        popcli();                                                                           \
        cpu_pause();                                                                        \
        pushcli();                                                                          \
    }                                                                                       \
    barrier();                                                                              \
    int locked = (lk)->s_tid ? (lk)->s_tid == thread_self() : (lk)->s_apicid == getcpuid(); \
    locked = (lk)->s_lock && locked;                                                        \
    atomic_clear(&(lk)->s_guard);                                                           \
    popcli();                                                                               \
    locked;                                                                                 \
})

#define spin_assert_locked(lk) ({                                     \
    assert_msg(                                                       \
        spin_islocked((lk)),                                          \
        "%s:%d: current[tid: %d, cpu: %d] "                           \
        "ret-> %p, spinlock MUST be held!\n",                         \
        __FILE__, __LINE__, thread_self(), getcpuid(), __retaddr(0)); \
})

#define spin_lock(lk) ({                                                 \
    spin_assert(lk);                                                     \
    pushcli();                                                           \
    loop()                                                               \
    {                                                                    \
        while (atomic_test_and_set(&(lk)->s_guard))                      \
        {                                                                \
            popcli();                                                    \
            cpu_pause();                                                 \
            pushcli();                                                   \
        }                                                                \
        barrier();                                                       \
        if ((lk)->s_lock == 0)                                           \
            break;                                                       \
        int self = (lk)->s_tid ? (lk)->s_tid == thread_self()            \
                               : (lk)->s_apicid == getcpuid();           \
        assert_msg(                                                      \
            self == 0,                                                   \
            "%s:%d: cpu: %d, state[tid: %d, cpu: %d, ret -> %p] "        \
            "Spinlock held at [%s:%d].\n",                               \
            __FILE__, __LINE__, getcpuid(), (lk)->s_tid, (lk)->s_apicid, \
            __retaddr(0), (lk)->s_file, (lk)->s_line);                   \
        atomic_clear(&(lk)->s_guard);                                    \
    }                                                                    \
    (lk)->s_lock = 1;                                                    \
    (lk)->s_line = __LINE__;                                             \
    (lk)->s_file = __FILE__;                                             \
    (lk)->s_apicid = getcpuid();                                         \
    (lk)->s_tid = thread_self();                                         \
    barrier();                                                           \
    atomic_clear(&(lk)->s_guard);                                        \
})

#define spin_unlock(lk) ({                                                 \
    spin_assert(lk);                                                       \
    pushcli();                                                             \
    while (atomic_test_and_set(&(lk)->s_guard))                            \
    {                                                                      \
        popcli();                                                          \
        cpu_pause();                                                       \
        pushcli();                                                         \
    }                                                                      \
    barrier();                                                             \
    int self = (lk)->s_tid ? (lk)->s_tid == thread_self()                  \
                           : (lk)->s_apicid == getcpuid();                 \
    assert_msg(                                                            \
        (self && (lk)->s_lock),                                            \
        "%s:%d: current[tid: %d, cpu: %d]"                                 \
        "Spinlock not held"                                                \
        " state[tid: %d, cpu: %d, %s at %s:%d: ret -> %p]",                \
        __FILE__, __LINE__, thread_self(), getcpuid(),                     \
        (lk)->s_tid, (lk)->s_apicid, (lk)->s_lock ? "locked" : "unlocked", \
        (lk)->s_file ? (lk)->s_file : "n/a", (lk)->s_line, __retaddr(0));  \
    (lk)->s_lock = 0;                                                      \
    (lk)->s_line = 0;                                                      \
    (lk)->s_file = NULL;                                                   \
    (lk)->s_apicid = -1;                                                   \
    (lk)->s_tid = 0;                                                       \
    barrier();                                                             \
    atomic_clear(&(lk)->s_guard);                                          \
    popcli();                                                              \
    popcli();                                                              \
})
