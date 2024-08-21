#pragma once

#include <arch/thread.h>
#include <core/types.h>
#include <core/spinlock.h>

typedef enum tstate_t {
    EMBRYO,
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED,
} tstate_t;

#define THREAD_USER         BS(0)    // thread is USER
#define THREAD_FPU          BS(1)    // thread is FPU
#define THREAD_DETACH       BS(2)    // thread is DETACHED
#define THREAD_MAIN         BS(3)    // thread is mainthread of a thread group.
#define THREAD_ISLEEP       BS(4)    // thread is in ISLEEP state.
#define THREAD_STOPPED      BS(5)    // thread is STOPPED

typedef struct thread_t {
    tid_t           t_tid;  // thread ID

    u64             t_flags;
    tstate_t        t_state;   

    tid_t           t_kid;  // killer ID
    u64             t_exit_code;

    spinlock_t      t_lock;
    arch_thread_t   t_arch;
} __packed thread_t;

#define thread_assert(t)            ({ assert(t, "No thread."); })

#define thread_lock(t)              ({ thread_assert(t); spin_lock(&(t)->t_lock); })
#define thread_unlock(t)            ({ thread_assert(t); spin_unlock(&(t)->t_lock); })
#define thread_islocked(t)          ({ thread_assert(t); spin_islocked(&(t)->t_lock); })
#define thread_assert_locked(t)     ({ thread_assert(t); spin_assert_locked(&(t)->t_lock); })

#define thread_getstate(t)          ({ thread_assert_locked(t); (t)->t_state; })
#define thread_setstate(t, s)       ({ thread_assert_locked(t); (t)->t_state = (s); })

#define thread_setflags(t, f)       ({ thread_assert_locked(t); (t)->t_flags |= (f); })
#define thread_testflags(t, f)      ({ thread_assert_locked(t); (t)->t_flags &  (f); })
#define thread_maskflags(t, f)      ({ thread_assert_locked(t); (t)->t_flags &=~(f); })

#define thread_isuser(t)            ({ thread_testflags(f, THREAD_USER); })
#define thread_hasfpu(t)            ({ thread_testflags(f, THREAD_FPU); })
#define thread_isdetached(t)        ({ thread_testflags(t, THREAD_DETACH); })
#define thread_ismain(t)            ({ thread_testflags(f, THREAD_MAIN); })
#define thread_isisleep(t)          ({ thread_testflags(t, THREAD_ISLEEP); })
#define thread_isstopped(t)         ({ thread_testflags(f, THREAD_STOPPED); })

#define current_assert()            ({ thread_assert(current); })
#define current_lock()              ({ thread_lock(current); })
#define current_unlock()            ({ thread_unlock(current); })
#define current_islocked()          ({ thread_islocked(current); })
#define current_assert_locked()     ({ thread_assert_locked(current); })

#define current_getstate()          ({ thread_getstate(current); })
#define current_setstate(s)         ({ thread_setstate(current, s); })

#define current_setflags(f)         ({ thread_setflags(current, f); })
#define current_testflags(f)        ({ thread_testflags(current, f); })
#define current_maskflags(f)        ({ thread_maskflags(current, f); })

#define current_isuser()            ({ thread_isuser(current); })
#define current_hasfpu()            ({ thread_hasfpu(current); })
#define current_isdetached()        ({ thread_isdetached(current); })
#define current_ismain()            ({ thread_ismain(current); })
#define current_isisleep()          ({ thread_isisleep(current); })
#define current_isstopped()         ({ thread_isstopped(current); })

/// get thread ID
/// returns tid of 'thread'
/// or 0 if thread is null is running.
extern tid_t thread_gettid(thread_t *thread);

/// get thread ID
/// call thread_gettid()
extern tid_t gettid(void);