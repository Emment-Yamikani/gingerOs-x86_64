#pragma once

#include <core/spinlock.h>
#include <ds/queue.h>
#include <sys/thread.h>

typedef struct mutex_t {
    u64         mtx_lock;
    queue_t     mtx_queue;
    thread_t    *mtx_thread;
    char        *mtx_file;
    char        *mtx_func;
    int         mtx_line;
    spinlock_t  mtx_gaurd;
} mutex_t;

#define MUTEX_INIT()    ((mutex_t){0})
#define MUTEX_PTR()     (&MUTEX_INIT())
#define MUTEX(__mtx_name)     mutex_t *__mtx_name = MUTEX_PTR()

/// mutex_assert
#define mutex_assert(mtx)           ({ assert(mtx, "Not a valid 'mutex'."); })

/// mutex_islocked
#define mutex_islocked(mtx) ({                                          \
    mutex_assert(mtx);                                                  \
    spin_lock(&(mtx)->mtx_gaurd);                                       \
    int locked = ((mtx)->mtx_lock && ((mtx)->mtx_thread == current));   \
    spin_unlock(&(mtx)->mtx_gaurd);                                     \
    locked;                                                             \
})

/// mutex_assert_locked
#define mutex_assert_locked(mtx) ({                                               \
    mutex_assert(mtx);                                                            \
    spin_lock(&(mtx)->mtx_gaurd);                                                 \
    int locked = ((mtx)->mtx_lock && ((mtx)->mtx_thread == current));             \
    assert_msg(locked, "%s@%s:%d: [tid:%d:pid:%d on cpu:%d] must hold mutex.\n",  \
               __func__, __FILE__, __LINE__, gettid(), getpid(), getcpuid());     \
    spin_unlock(&(mtx)->mtx_gaurd);                                               \
})

/// mutex_lock
#define mutex_lock(mtx) ({                                                                       \
    mutex_assert(mtx);                                                                           \
    spin_lock(&(mtx)->mtx_gaurd);                                                                \
    int locked = ((mtx)->mtx_lock && ((mtx)->mtx_thread == current));                            \
    assert_msg(!locked, "%s@%s:%d: [tid:%d:pid:%d on cpu:%d] mutex already held @ %s:%s:%d.\n",  \
               __func__, __FILE__, __LINE__, gettid(), getpid(), getcpuid(),                     \
               (mtx)->mtx_func, (mtx)->mtx_file, (mtx)->mtx_line);                               \
    ++(mtx)->mtx_lock;                                                                           \
    if ((mtx)->mtx_lock > 1) {                                                                   \
        current_lock();                                                                          \
        sched_sleep(&(mtx)->mtx_queue, T_ISLEEP, &(mtx)->mtx_gaurd);                             \
        current_unlock();                                                                        \
    } else if ((mtx)->mtx_lock == 0)                                                             \
        panic("%s@%s:%d: mutex->mtx_lock == 0??", __func__, __FILE__, __LINE__);                 \
    (mtx)->mtx_thread   = current;                                                               \
    (mtx)->mtx_line     = __LINE__;                                                              \
    (mtx)->mtx_file     = (char *)__FILE__;                                                      \
    (mtx)->mtx_func     = (char *)__func__;                                                      \
    spin_unlock(&(mtx)->mtx_gaurd);                                                              \
})

/// mutex_try_lock
#define mutex_try_lock(mtx) ({                                                                   \
    mutex_assert(mtx);                                                                           \
    spin_lock(&(mtx)->mtx_gaurd);                                                                \
    int locked = !((mtx)->mtx_lock && ((mtx)->mtx_thread == current));                           \
    assert_msg(locked, "%s@%s:%d: [tid:%d:pid:%d on cpu:%d] mutex already held @ %s:%s:%d.\n",   \
               __func__, __FILE__, __LINE__, gettid(), getpid(), getcpuid(),                     \
               (mtx)->mtx_func, (mtx)->mtx_file, (mtx)->mtx_line);                               \
    if ((mtx)->mtx_lock == 0) {                                                                  \
        (mtx)->mtx_lock     =   1;                                                               \
        (mtx)->mtx_thread   = current;                                                           \
        (mtx)->mtx_line     = __LINE__;                                                          \
        (mtx)->mtx_file     = (char *)__FILE__;                                                  \
        (mtx)->mtx_func     = (char *)__func__;                                                  \
        locked = 1;                                                                              \
    }                                                                                            \
    spin_unlock(&(mtx)->mtx_gaurd);                                                              \
    locked;                                                                                      \
})

/// mutex_unlock
#define mutex_unlock(mtx) ({                                                     \
    mutex_assert(mtx);                                                           \
    spin_lock(&(mtx)->mtx_gaurd);                                                \
    int locked = ((mtx)->mtx_lock && ((mtx)->mtx_thread == current));            \
    assert_msg(locked, "%s@%s:%d: [tid:%d:pid:%d on cpu:%d] must hold mutex.\n", \
               __func__, __FILE__, __LINE__, gettid(), getpid(), getcpuid());    \
    if (--(mtx)->mtx_lock > 0)                                                   \
        sched_wake1(&(mtx)->mtx_queue);                                          \
    (mtx)->mtx_line = 0;                                                         \
    (mtx)->mtx_thread = NULL;                                                    \
    (mtx)->mtx_file = NULL;                                                      \
    (mtx)->mtx_func = NULL;                                                      \
    spin_unlock(&(mtx)->mtx_gaurd);                                              \
})
