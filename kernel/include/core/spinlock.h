#pragma once

#include <sys/system.h>
#include <sync/atomic.h>
#include <sync/preempt.h>
#include <lib/printk.h>
#include <lib/types.h>
#include <lib/stdint.h>

extern tid_t thread_self(void);

typedef struct {
    bool    is_threaded;
    int     id; // Thread ID or CPU ID
} spinlock_owner_t;

typedef struct __spinlock_t {
    spinlock_owner_t   s_owner;
    bool                s_lock;
    bool                s_guard;

    // Debugging info //

    char                *s_file;
    int                 s_line;
} spinlock_t;

#define MAX_SPIN_RETRIES 1000

#define SPINLOCK_INIT() ((spinlock_t){       \
    .s_guard = 0,                            \
    .s_lock = 0,                             \
    .s_owner = {.id = -1, .is_threaded = 0}, \
    .s_file = NULL,                          \
    .s_line = 0})

#define SPINLOCK_NEW() (&SPINLOCK_INIT())
#define SPINLOCK(name) spinlock_t *name = SPINLOCK_NEW()

#define spin_assert(lk) ({ assert(lk, "No spinlock"); })

#define spin_try_guard(lk) ({                          \
    int success = 0;                                   \
    barrier();                                         \
    for (int i = 0; i < MAX_SPIN_RETRIES; i++) {       \
        if (!atomic_test_and_set(&(lk)->s_guard)) {    \
            success = 1;                               \
            break;                                     \
        }                                              \
        cpu_pause();                                   \
    }                                                  \
    barrier();                                         \
    success;                                           \
})

#define spin_lock(lk) ({                                                                                                \
    spin_assert(lk);                                                                                                    \
    pushcli();                                                                                                          \
    loop() {                                                                                                            \
        if (spin_try_guard(lk)) {                                                                                       \
            if ((lk)->s_lock == 0)                                                                                      \
                break;                                                                                                  \
            bool self = (lk)->s_owner.is_threaded ? (lk)->s_owner.id == thread_self() : (lk)->s_owner.id == getcpuid(); \
            assert_msg(self == 0,                                                                                       \
                       "PANIC: %s(): %s:%d: [cpu: %d, tid: %d, ret: %p] Recursive lock detected.\n"                     \
                       "%s:%d acquired lock @ [%s:%d].\n",                                                              \
                       __func__, __FILE__,                                                                              \
                       __LINE__, getcpuid(), thread_self(), __retaddr(0),                                               \
                       (lk)->s_owner.is_threaded ? "tid" : "cpu", (lk)->s_owner.id, (lk)->s_file, (lk)->s_line);        \
            atomic_clear(&(lk)->s_guard);                                                                               \
        }                                                                                                               \
    }                                                                                                                   \
    (lk)->s_lock = 1;                                                                                                   \
    (lk)->s_file = __FILE__;                                                                                            \
    (lk)->s_line = __LINE__;                                                                                            \
    (lk)->s_owner.is_threaded = !!current;                                                                              \
    (lk)->s_owner.id = !!current ? thread_self() : getcpuid();                                                          \
    barrier();                                                                                                          \
    atomic_clear(&(lk)->s_guard);                                                                                       \
})

#define spin_unlock(lk) ({                                                                                      \
    spin_assert(lk);                                                                                            \
    pushcli();                                                                                                  \
    while (!spin_try_guard(lk)) {                                                                               \
    }                                                                                                           \
    bool self = (lk)->s_owner.is_threaded ? (lk)->s_owner.id == thread_self() : (lk)->s_owner.id == getcpuid(); \
    assert_msg(self && (lk)->s_lock,                                                                            \
               "PANIC: %s(): %s:%d: [cpu: %d, tid: %d, ret: %p] Does not own lock.\n"                           \
               "State [%s %s:%d] @ [%s:%d].\n",                                                                 \
               __func__, __FILE__, __LINE__,                                                                    \
               getcpuid(), thread_self(), __retaddr(0), (lk)->s_lock ? "locked" : "unlocked",                   \
               (lk)->s_lock ? (lk)->s_owner.is_threaded ? "tid" : "cpu" : "n/a",                                \
               (lk)->s_lock ? (lk)->s_owner.id : -1, (lk)->s_file ? (lk)->s_file : "n/a", (lk)->s_line);        \
    (lk)->s_lock = 0;                                                                                           \
    (lk)->s_line = 0;                                                                                           \
    (lk)->s_file = NULL;                                                                                        \
    (lk)->s_owner.id = -1,                                                                                      \
    (lk)->s_owner.is_threaded = 0;                                                                              \
    barrier();                                                                                                  \
    atomic_clear(&(lk)->s_guard);                                                                               \
    popcli();                                                                                                   \
    popcli();                                                                                                   \
})

#define spin_islocked(lk) ({                                                                                                         \
    spin_assert(lk);                                                                                                                 \
    pushcli();                                                                                                                       \
    while (!spin_try_guard(lk)) {                                                                                                    \
    }                                                                                                                                \
    bool locked = (lk)->s_lock ? (lk)->s_owner.is_threaded ? (lk)->s_owner.id == thread_self() : (lk)->s_owner.id == getcpuid() : 0; \
    atomic_clear(&(lk)->s_guard);                                                                                                    \
    popcli();                                                                                                                        \
    locked;                                                                                                                          \
})

#define spin_assert_locked(lk) ({                                                      \
    assert_msg(spin_islocked(lk),                                                      \
               "%s(): %s:%d: [cpu: %d, tid: %d] ret: %p. Must acquired spinlock.\n",   \
               __func__, __FILE__, __LINE__, getcpuid(), thread_self(), __retaddr(0)); \
})
