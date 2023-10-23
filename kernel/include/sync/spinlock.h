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
    uint8_t s_lock;
    int s_line;
    cpu_t *s_cpu;
    char *s_file;
    thread_t *s_thread;
    void *s_retaddr;
    uint8_t s_guard;
} spinlock_t;

#define SPINLOCK_INIT() ((spinlock_t){ \
    .s_line = 0,                       \
    .s_lock = 0,                       \
    .s_file = NULL,                    \
    .s_thread = NULL,                  \
    .s_retaddr = NULL,                 \
    .s_cpu = NULL,                     \
})

#define SPINLOCK_NEW()  (&SPINLOCK_INIT())

// assert_msg lock pointer
#define spin_assert_msg(lk, __msg__) ({ assert_msg((lk), __msg__); })

// assert_msg lock pointer
#define spin_assert(lk) ({ spin_assert_msg((lk), "no spinlock!"); })

// assert_msg spinlock acquisition
#define spin_assert_locked(lk) ({                                                 \
    assert_msg(spin_islocked((lk)),                                               \
               "%s:%d: CPU%d: current: %d: s_retaddr: %lX: spinlock not held!\n", \
               __FILE__, __LINE__, cpu_id, thread_self(), __retaddr(0));          \
})

#define spin_debug(lk) ({                                       \
    spin_assert(lk);                                            \
    pushcli();                                                  \
    printk("%s:%d: in %s, CPU[%d] state: %s,"                   \
           " cpu: %d, s_thread: %d\n",                          \
           __FILE__, __LINE__, __func__, cpu_id,                \
           atomic_read(&(lk)->lock) ? "locked" : "unlocked",    \
           (lk)->s_cpu->apicID, thread_gettid((lk)->s_thread)); \
    popcli();                                                   \
})

// acquire spinlock
#define spin_lock(lk) ({                                                             \
    spin_assert(lk);                                                                 \
    for (;;)                                                                         \
    {                                                                                \
        pushcli();                                                                   \
        barrier();                                                                   \
        while (atomic_test_and_set(&(lk)->s_guard))                                  \
        {                                                                            \
            popcli();                                                                \
            cpu_pause();                                                             \
            pushcli();                                                               \
        }                                                                            \
        if ((lk)->s_lock)                                                            \
        {                                                                            \
            assert_msg(!(((lk)->s_thread ? (lk)->s_thread == current                 \
                                         : (lk)->s_cpu == cpu) &&                    \
                         1),                                                         \
                       "%s:%d: cpu[%d] thread[tid:%d::cpu:%d] state[%s] current[%d]" \
                       " Spinlock already held at [%s:%ld:%p].\n",                   \
                       __FILE__, __LINE__, cpu_id, thread_gettid((lk)->s_thread),    \
                       (lk)->s_cpu ? (lk)->s_cpu->apicID : -1,                       \
                       (lk)->s_lock ? "locked" : "unlocked", thread_self(),          \
                       (lk)->s_file, (lk)->s_line, (lk)->s_retaddr);                 \
            atomic_clear(&(lk)->s_guard);                                            \
            popcli();                                                                \
        }                                                                            \
        else                                                                         \
            break;                                                                   \
    }                                                                                \
    (lk)->s_lock = 1;                                                                \
    (lk)->s_line = __LINE__;                                                         \
    (lk)->s_file = __FILE__;                                                         \
    (lk)->s_cpu = cpu;                                                               \
    (lk)->s_thread = current;                                                        \
    (lk)->s_retaddr = __retaddr(0);                                                  \
    atomic_clear(&(lk)->s_guard);                                                    \
})

// release spinlock
#define spin_unlock(lk) ({                                                   \
    spin_assert(lk);                                                         \
    pushcli();                                                               \
    barrier();                                                               \
    while (atomic_test_and_set(&(lk)->s_guard))                              \
    {                                                                        \
        popcli();                                                            \
        cpu_pause();                                                         \
        pushcli();                                                           \
    }                                                                        \
    assert_msg((((lk)->s_thread ? (lk)->s_thread == current                  \
                                : (lk)->s_cpu == cpu) &&                     \
                (lk)->s_lock),                                               \
               "%s:%d: cpu[%d] thread[tid:%d::cpu:%d] state[%s] current[%d]" \
               " Spinlock not held.",                                        \
               __FILE__, __LINE__, cpu_id,                                   \
               thread_gettid((lk)->s_thread),                                \
               (lk)->s_cpu ? (lk)->s_cpu->apicID : -1,                       \
               (lk)->s_lock ? "locked" : "unlocked", thread_self());         \
    (lk)->s_line = 0;                                                        \
    (lk)->s_retaddr = 0;                                                     \
    (lk)->s_file = NULL;                                                     \
    (lk)->s_thread = NULL;                                                   \
    (lk)->s_cpu = NULL;                                                      \
    (lk)->s_lock = 0;                                                        \
    barrier();                                                               \
    atomic_clear(&(lk)->s_guard);                                            \
    popcli(); /*Reverse pushcli() when holding (lk)->s_guard*/               \
    popcli(); /*Reverse pushcli() when holding this spinlock*/               \
})

/**
 * @brief try to acquire a spinlock
 *
 * @param lk
 * @return int '1' if acquisition was successful(lock wasn't held before) and '0' if acquisition failed(lock is already held)
 */
#define spin_trylock(lk) ({                                                      \
    spin_assert(lk);                                                             \
    int held = 0;                                                                \
    pushcli();                                                                   \
    barrier();                                                                   \
    while (atomic_test_and_set(&(lk)->s_guard))                                  \
    {                                                                            \
        popcli();                                                                \
        cpu_pause();                                                             \
        pushcli();                                                               \
    }                                                                            \
    if ((lk)->s_lock)                                                            \
    {                                                                            \
        assert_msg((!((lk)->s_thread ? (lk)->s_thread == current                 \
                                     : (lk)->s_cpu == cpu) &&                    \
                    1),                                                          \
                   "%s:%d: cpu[%d] thread[tid:%d::cpu:%d] state[%s] current[%d]" \
                   " Spinlock already held.",                                    \
                   __FILE__, __LINE__, cpu_id,                                   \
                   thread_gettid((lk)->s_thread),                                \
                   (lk)->s_cpu ? (lk)->s_cpu->apicID : -1,                       \
                   (lk)->s_lock ? "locked" : "unlocked", thread_self());         \
        held = 0;                                                                \
    }                                                                            \
    else                                                                         \
    {                                                                            \
        pushcli();                                                               \
        (lk)->s_lock = 1;                                                        \
        (lk)->s_line = __LINE__;                                                 \
        (lk)->s_file = __FILE__;                                                 \
        (lk)->s_cpu = cpu;                                                       \
        (lk)->s_thread = current;                                                \
        (lk)->s_retaddr = __retaddr(0);                                          \
        held = 1;                                                                \
    }                                                                            \
    atomic_clear(&(lk)->s_guard);                                                \
    popcli();                                                                    \
    held;                                                                        \
})

#define spin_islocked(lk) ({                           \
    int held = 0;                                      \
    spin_assert(lk);                                   \
    pushcli();                                         \
    barrier();                                         \
    while (atomic_test_and_set(&(lk)->s_guard))        \
    {                                                  \
        popcli();                                      \
        cpu_pause();                                   \
        pushcli();                                     \
    }                                                  \
    held = ((lk)->s_thread ? (lk)->s_thread == current \
                           : (lk)->s_cpu == cpu) &&    \
           (lk)->s_lock;                               \
    atomic_clear(&(lk)->s_guard);                      \
    popcli();                                          \
    held;                                              \
})
