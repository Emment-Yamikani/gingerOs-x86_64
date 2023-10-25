#pragma once

#include <ds/queue.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <sync/spinlock.h>

typedef struct mutex_t mutex_t;
typedef struct mutex_t {
    uint8_t     mtx_lock;       // mutex lock ('1' if acquired, '0' if free).
    char        *mtx_file;
    size_t      mtx_line;
    char        *mtx_func;
    queue_t     mtx_waiters;    // queue for contenders of this mutex
    thread_t    *mtx_thread;    // thread currently holding the mutex.
    spinlock_t  mtx_guard;      // spinlock guard for this mutex.
} mutex_t;

#define MUTEX_INIT()                ((mutex_t){0})
#define MUTEX_NEW()                 (&MUTEX_INIT())

#define mutex_assert(mtx)               ({assert(mtx, "No mutex");})
#define mutex_guard_lock(mtx)           ({mutex_assert(mtx); spin_lock(&(mtx)->mtx_guard); })
#define mutex_guard_unlock(mtx)         ({mutex_assert(mtx); spin_unlock(&(mtx)->mtx_guard); })
#define mutext_guard_islocked(mtx)      ({mutex_assert(mtx); spin_islocked(&(mtx)->mtx_guard); })
#define mutex_guard_assert_locked(mtx)  ({mutex_assert(mtx); spin_assert_locked(&(mtx)->mtx_guard); })

#define mutex_islocked(mtx) ({               \
    int locked = 0;                          \
    mutex_guard_lock(mtx);                   \
    locked = (mtx)->mtx_lock &&              \
             ((mtx)->mtx_thread == current); \
    mutex_guard_unlock(mtx);                 \
    locked;                                  \
})

#define mutex_trylock(mtx) ({                            \
    int locked = 0;                                      \
    mutex_guard_lock(mtx);                               \
    assert_msg(!((mtx)->mtx_lock &&                      \
                 ((mtx)->mtx_thread == current)),        \
               "Mutex already held at [\e[0;04m%s\e[0m:" \
               "\e[0;02m%d\e[0m in \e[0;011m%s()\e[0m;"  \
               "]: tid: \e[0;013m%ld\e[0m\n",            \
               (mtx)->mtx_file, (mtx)->mtx_line,         \
               (mtx)->mtx_func, thread_gettid(current)); \
    if ((mtx)->mtx_lock == 0) {                          \
        (mtx)->mtx_lock = 1;                             \
        (mtx)->mtx_thread = current;                     \
        (mtx)->mtx_file = __FILE__;                      \
        (mtx)->mtx_line = __LINE__;                      \
        (mtx)->mtx_func = (char *)__func__;              \
        locked = 1;                                      \
    }                                                    \
    mutex_guard_unlock(mtx);                             \
    locked;                                              \
})

#define mutex_lock(mtx) ({                                \
    mutex_guard_lock(mtx);                                \
    assert_msg(!((mtx)->mtx_lock &&                       \
                 ((mtx)->mtx_thread == current)),         \
               "Mutex already held at [\e[0;016m%s\e[0m:" \
               "\e[0;02m%d\e[0m in \e[0;015m%s()\e[0m;"   \
               "]: tid: \e[0;013m%ld\e[0m\n",             \
               (mtx)->mtx_file, (mtx)->mtx_line,          \
               (mtx)->mtx_func, thread_gettid(current));  \
    if ((mtx)->mtx_lock == 0)                             \
        (mtx)->mtx_lock = 1;                              \
    else {                                                \
        current_lock();                                   \
        sched_sleep(&(mtx)->mtx_waiters,                  \
                    T_ISLEEP, &(mtx)->mtx_guard);         \
        current_unlock();                                 \
    }                                                     \
    (mtx)->mtx_thread = current;                          \
    (mtx)->mtx_file = __FILE__;                           \
    (mtx)->mtx_line = __LINE__;                           \
    (mtx)->mtx_func = (char *)__func__;                   \
    mutex_guard_unlock(mtx);                              \
})

#define mutex_unlock(mtx) ({                     \
    mutex_guard_lock(mtx);                       \
    assert_msg(((mtx)->mtx_lock &&               \
                ((mtx)->mtx_thread == current)), \
               "Mutex not held: tid: "           \
               "\e[0;013m%ld\e[0m\n",            \
               thread_gettid(current));          \
    if (sched_wake1(&(mtx)->mtx_waiters))        \
        (mtx)->mtx_lock = 0;                     \
    (mtx)->mtx_thread = NULL;                    \
    (mtx)->mtx_file = NULL;                      \
    (mtx)->mtx_line = 0;                         \
    (mtx)->mtx_func = NULL;                      \
    mutex_guard_unlock(mtx);                     \
})
