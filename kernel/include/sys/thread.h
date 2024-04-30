#pragma once

#include <arch/cpu.h>
#include <arch/paging.h>
#include <arch/thread.h>
#include <bits/errno.h>
#include <ds/queue.h>
#include <fs/file.h>
#include <ginger/jiffies.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <lib/types.h>
#include <mm/mmap.h>
#include <sync/cond.h>
#include <sync/spinlock.h>
#include <sys/_signal.h>
#include <sys/system.h>
#include <sys/tgroup.h>
#include <sys/_time.h>

typedef enum tstate_t {
    T_EMBRYO,       // Embryo.
    T_READY,        // Ready.
    T_RUNNING,      // Running.
    T_ISLEEP,       // Interruptable sleep.
    T_USLEEP,       // Uninterruptable sleep.
    T_STOPPED,      // Stopped.
    T_TERMINATED,   // Terminated.
    T_ZOMBIE,       // Zombie.
} tstate_t;

extern const char *t_states[];
extern char *tget_state(const tstate_t st);

typedef struct proc proc_t;

typedef struct thread_attr_t {
    int         detachstate;
    usize       guardsz;
    uintptr_t   stackaddr;
    usize       stacksz;
} thread_attr_t;

typedef struct thread_sched_t {
    time_t      ts_ctime;       // Thread ceation time.
    time_t      ts_cpu_time;    // CPU time in jiffies(n seconds = (jiffy * (HZ_TO_ns(SYS_HZ) / seconds_TO_ns(1))) ).
    time_t      ts_timeslice;   // Quantum of CPU time for which this thread is allowed to run.
    time_t      ts_total_time;  // Total time this thread has run.
    time_t      ts_last_sched;  // Last time this thread was scheduled to run.
    cpu_t       *ts_processor;  // Current Processor for which this thread has affinity.
    struct {
        enum{
            SOFT_AFFINITY = 0,  /*soft affinity for the cpu*/
            HARD_AFFINITY = 1,  /*hard affinity for the cpu*/
        } type;                 // Type of affinity (SOFT or HARD).
        flags32_t   cpu_set;    // cpu set for which thread can have affinity for.
    } ts_affinity;
    atomic_t    ts_priority;            // Thread scheduling Priority.
} thread_sched_t;

#define sched_DEFAULT() (thread_sched_t){0}


typedef struct sleep_attr_t {
    queue_node_t    *node;              // thread's sleep node.
    queue_t         *queue;             // thread's sleep queue.
    spinlock_t      *guard;             // non-null if sleep queue is associated with a guard lock.
} sleep_attr_t;

typedef struct __thread_shared_t {
    tid_t           tgid;
    file_ctx_t      *fctx;
    cred_t          *cred;
    queue_t         *threads;
    sig_desc_t      *sig_desc;
    spinlock_t      lock;
} thread_shared_t;

typedef struct __thread_t {
    tid_t           t_tid;              // thread ID.
    tid_t           t_tgid;
    tid_t           t_ktid;             // killer thread ID for thread that killed this thread.
    thread_entry_t  t_entry;            // thread entry point.

    isize           t_refcnt;           // thread's reference count.

    proc_t          *t_owner;           // thread's owner process.

    uintptr_t       t_exit;             // thread exit code.
    tstate_t        t_state;            // thread's execution state.
    atomic_t        t_flags;            // thread's flags.
    uintptr_t       t_errno;            // thread's errno.

    sigset_t        t_sigmask;          // thread's masked signal set.
    queue_t         t_sigqueue[NSIG];   // queues of siginfo_t * pointers for each pending signal instance.

    void            *t_simd_ctx;        // thread's Single Instruction Multiple Data (SIMD) context.
    mmap_t          *t_mmap;            // thread's process virtual address space.
    sleep_attr_t    t_sleep;            // struct describing sleep attributes for this thread.
    thread_sched_t  t_sched;            // struct describing scheduler attributes for this thread.

    // Shared data among threads of the same group.

    cred_t          *t_cred;            // credentials for this thread's tgroup.
    file_ctx_t      *t_fctx;            // Pointer to file context for this thread's tgroup.
    queue_t         *t_tgroup;          // queue of all threads in the logical thread group.
    sig_desc_t      *t_sigdesc;         // thread group wide-signal description.

    ///////////////////////////////////////////////////////////

    cond_t          t_wait;             // thread conditional wait variable.
    arch_thread_t   t_arch;             // architecture-specific thread struct.
    queue_t         t_queues;           // queues on which this thread resides.

    spinlock_t      t_lock;             // lock to synchronize access to this struct.

    // Misc. debug information.
    
    tid_t           t_statetid;
    isize           t_stateline;
    char            *t_statefile;
} __aligned(16) thread_t;

typedef struct {
    tid_t           ti_tid;
    tid_t           ti_ktid;
    tstate_t        ti_state;
    thread_sched_t  ti_sched;
    uintptr_t       ti_errno;
    uintptr_t       ti_exit;
    atomic_t        ti_flags;
} thread_info_t;

#define THREAD_USER                     BS(0)   // thread is a user thread.
#define THREAD_KILLED                   BS(1)   // thread was killed by another thread.
#define THREAD_PARK                     BS(2)   // thread has the park flag set.
#define THREAD_WAKE                     BS(3)   // thread has the wakeup flag set.
#define THREAD_HANDLING_SIG             BS(4)   // thread is currently handling a signal.
#define THREAD_DETACHED                 BS(5)   // free resources allocated to this thread imediately to terminates.
#define THREAD_STOP                     BS(6)   // thread stop.
#define THREAD_SIMD_DIRTY               BS(7)   // thread's SIMD context is dirty and must be save on context swtich.
#define THREAD_ISMAIN                   BS(8)   // thread is a main thread in the tgroup.
#define THREAD_ISLAST                   BS(9)   // thread is the last thread in the troup.
#define THREAD_CONTINUE                 BS(10)  // thread has been continued from a stopped state.
#define THREAD_USING_SSE                BS(11)  // thread is using SSE extensions if this flags is set, FPU otherwise.
#define THREAD_EXITING                  BS(12)  // thread is exiting.


#define thread_assert(t)                ({ assert(t, "No thread pointer\n");})
#define thread_lock(t)                  ({ thread_assert(t); spin_lock(&((t)->t_lock)); })
#define thread_unlock(t)                ({ thread_assert(t); spin_unlock(&((t)->t_lock)); })
#define thread_islocked(t)              ({ thread_assert(t); spin_islocked(&((t)->t_lock)); })
#define thread_assert_locked(t)         ({ thread_assert(t); spin_assert_locked(&((t)->t_lock)); })

#define thread_grabref(thread)          ({ thread_assert_locked(thread); (thread)->t_refcnt; })
#define thread_putref(thread)           ({ thread_assert_locked(thread); (thread)->t_refcnt--; })
#define thread_getref(thread)           ({ thread_assert_locked(thread); (thread)->t_refcnt++; thread; })
/**
 * @brief drop the reference to the thread pointer
 * and release the lock.
 * this doesn't destroy the thread struct is t_refcnt <= 0.
 */
#define thread_release(thread)          ({thread_assert_locked(thread); thread_putref(thread); thread_unlock(thread); })
#define thread_isstate(t, state)        ({ thread_assert_locked(t); ((t)->t_state == (state)); })
#define thread_isembryo(t)              ({ thread_isstate(t, T_EMBRYO); })
#define thread_isready(t)               ({ thread_isstate(t, T_READY); })
#define thread_isisleep(t)              ({ thread_isstate(t, T_ISLEEP); })
#define thread_isusleep(t)              ({ thread_isstate(t, T_USLEEP); })
#define thread_isrunning(t)             ({ thread_isstate(t, T_RUNNING); })
#define thread_isstopped(t)             ({ thread_isstate(t, T_STOPPED); })
#define thread_iszombie(t)              ({ thread_isstate(t, T_ZOMBIE); })
#define thread_isterminated(t)          ({ thread_isstate(t, T_TERMINATED); })
#define thread_testflags(t, flags)      ({ thread_assert_locked(t); atomic_read(&((t)->t_flags)) & (flags); })
#define thread_setflags(t, flags)       ({ thread_assert_locked(t); atomic_fetch_or(&((t)->t_flags), (flags)); })
#define thread_maskflags(t, flags)      ({ thread_assert_locked(t); atomic_fetch_and(&((t)->t_flags), ~(flags)); })
#define thread_getstate(t)              ({ thread_assert_locked(t); (t)->t_state; })
#define thread_enter_state(t, state) ({           \
    thread_assert_locked(t);                      \
    int err = 0;                                  \
    if ((state) < T_EMBRYO || (state) > T_ZOMBIE) \
        err = -EINVAL;                            \
    else                                          \
    {                                             \
        (t)->t_state = state;                     \
        (t)->t_statetid = thread_self();          \
        (t)->t_statefile = __FILE__;              \
        (t)->t_stateline = __LINE__;              \
    }                                             \
    err;                                          \
})

#define thread_ismain(t) ({                            \
    int locked = thread_islocked(t);                   \
    if (!locked)                                       \
        thread_lock(t);                                \
    int ismain = thread_testflags((t), THREAD_ISMAIN); \
    if (!locked)                                       \
        thread_unlock(t);                              \
    ismain;                                            \
})

#define thread_islast(t) ({                            \
    int locked = thread_islocked(t);                   \
    if (!locked)                                       \
        thread_lock(t);                                \
    int ismain = thread_testflags((t), THREAD_ISLAST); \
    if (!locked)                                       \
        thread_unlock(t);                              \
    ismain;                                            \
})

#define thread_iskilled(t) ({                          \
    int locked = thread_islocked(t);                   \
    if (!locked)                                       \
        thread_lock(t);                                \
    int killed = thread_testflags((t), THREAD_KILLED); \
    if (!locked)                                       \
        thread_unlock(t);                              \
    killed;                                            \
})

#define thread_ishandling_signal(t) ({                         \
    int locked = thread_islocked(t);                           \
    if (!locked)                                               \
        thread_lock(t);                                        \
    int handling = thread_testflags((t), THREAD_HANDLING_SIG); \
    if (!locked)                                               \
        thread_unlock(t);                                      \
    handling;                                                  \
})

#define thread_tgroup(t)                ({ thread_assert(t); (t)->t_tgroup; })
#define thread_tgroup_lock(t)           ({ tgroup_lock(thread_tgroup(t)); })
#define thread_tgroup_unlock(t)         ({ tgroup_unlock(thread_tgroup(t)); })
#define thread_tgroup_locked(t)         ({ tgroup_locked(thread_tgroup(t)); })

#define thread_isuser(t) ({                    \
    int locked = 0, user = 0;                  \
    if ((locked = !thread_islocked(t)))        \
        thread_lock(t);                        \
    user = thread_testflags((t), THREAD_USER); \
    if (locked)                                \
        thread_unlock(t);                      \
    user;                                      \
})

#define thread_isdetached(t)            ({ thread_testflags((t), THREAD_DETACHED); })
#define thread_issetwake(t)             ({ thread_testflags((t), THREAD_WAKE); })
#define thread_issetpark(t)             ({ thread_testflags((t), THREAD_PARK); })

#define thread_setmain(t) ({             \
    int locked = 0;                      \
    if ((locked = !thread_islocked(t)))  \
        thread_lock(t);                  \
    thread_setflags((t), THREAD_ISMAIN); \
    if (locked)                          \
        thread_unlock(t);                \
})

#define thread_setlast(t) ({             \
    int locked = 0;                      \
    if ((locked = !thread_islocked(t)))  \
        thread_lock(t);                  \
    thread_setflags((t), THREAD_ISLAST); \
    if (locked)                          \
        thread_unlock(t);                \
})

#define thread_setuser(t)               ({ thread_setflags((t), THREAD_USER); })
#define thread_setdetached(t)           ({ thread_setflags((t), THREAD_DETACHED); })
#define thread_setwake(t)               ({ thread_setflags((t), THREAD_WAKE); })
#define thread_setpark(t)               ({ thread_setflags((t), THREAD_PARK); })
#define thread_set_park_wake(t)         ({ thread_setflags((t), THREAD_WAKE | THREAD_PARK); })

#define thread_maskdetached(t)          ({ thread_maskflags((t), THREAD_DETACHED); })
#define thread_maskwake(t)              ({ thread_maskflags((t), THREAD_WAKE); })
#define thread_maskpark(t)              ({ thread_maskflags((t), THREAD_PARK); })
#define thread_mask_park_wake(t)        ({ thread_maskflags((t), THREAD_WAKE | THREAD_PARK); })

#define thread_issetpark_wake(t)        ({ thread_testflags((t), THREAD_WAKE | THREAD_PARK); })
#define thread_issimd_dirty(t)          ({ thread_testflags((t), THREAD_SIMD_DIRTY); })
#define thread_set_simd_dirty(t)        ({ thread_setflags((t), THREAD_SIMD_DIRTY); })
#define thread_mask_simd_dirty(t)       ({ thread_maskflags((t), THREAD_SIMD_DIRTY); })

#define current_assert()                ({ assert(current, "No current thread running"); })
#define current_lock()                  ({ thread_lock(current); })
#define current_unlock()                ({ thread_unlock(current); })
#define current_locked()                ({ thread_islocked(current); })
#define current_assert_locked()         ({ thread_assert_locked(current); })

#define current_tgroup()                ({ current ? thread_tgroup(current) : NULL; })
#define current_tgroup_lock()           ({ thread_tgroup_lock(current); })
#define current_tgroup_unlock()         ({ thread_tgroup_unlock(current); })
#define current_tgroup_locked()         ({ thread_tgroup_locked(current); })

#define current_setuser()               ({ thread_setuser(current); })
#define current_setdetached()           ({ thread_setdetached(current); })
#define current_setwake()               ({ thread_setwake(current); })
#define current_setpark()               ({ thread_setpark(current); })
#define current_set_park_wake()         ({ thread_set_park_wake(current); })

#define current_maskdetached()          ({ thread_maskdetached(current); })
#define current_maskwake()              ({ thread_maskwake(current); })
#define current_maskpark()              ({ thread_maskpark(current); })
#define current_mask_park_wake()        ({ thread_mask_park_wake(current); })

#define current_ismain()                ({ thread_ismain(current); })
#define current_isuser()                ({ thread_isuser(current); })
#define current_iskilled()              ({ thread_iskilled(current); })
#define current_issetwake()             ({ thread_issetwake(current); })
#define current_issetpark()             ({ thread_issetpark(current); })
#define current_issetpark_wake()        ({ thread_issetpark_wake(current); })
#define current_isdetached()            ({ thread_isdetached(current); })
#define current_ishandling()            ({ thread_ishandling_signal(current); })

#define current_isstate(state)          ({ thread_isstate(current, state); })
#define current_embryo()                ({ thread_isembryo(current); })
#define current_ready ()                ({ thread_isready(current); })
#define current_isisleep()              ({ thread_isisleep(current); })
#define current_isusleep()              ({ thread_isusleep(current); })
#define current_isrunning()             ({ thread_isrunning(current); })
#define current_isstopped()             ({ thread_isstopped(current); })
#define current_iszombie()              ({ thread_iszombie(current); })
#define current_isterminated()          ({ thread_isterminated(current); })
#define current_enter_state(state)      ({ thread_enter_state(current, (state)); })
#define current_getstate()              ({ thread_getstate(current); })

#define current_setflags(flags)         ({ thread_setflags(current, (flags)); })
#define current_testflags(flags)        ({ thread_testflags(current, (flags)); })
#define current_maskflags(flags)        ({ thread_maskflags(current, (flags)); })

#define current_issimd_dirty()          ({ thread_issimd_dirty(current); })
#define current_set_simd_dirty()        ({ thread_set_simd_dirty(current); })
#define current_mask_simd_dirty()       ({ thread_mask_simd_dirty(current); })

typedef struct {
    char            *thread_name;
    void            *thread_arg;
    void            *thread_entry;
    void            *thread_rsvd;
} builtin_thread_t;

extern builtin_thread_t __builtin_thrds[];
extern builtin_thread_t __builtin_thrds_end[];

#define BUILTIN_THREAD(name, entry, arg)          \
builtin_thread_t __used_section(.__builtin_thrds) \
    __thread_##name = {                           \
        .thread_name = #name,                     \
        .thread_arg = arg,                        \
        .thread_entry = entry,                    \
}

#define BUILTIN_THREAD_ANOUNCE(name)    ({ printk("\"%s\" thread [tid: %d] running...\n", name, thread_self()); })

#define STACKSZMIN      (KiB(32))
#define KSTACKSZ        (STACKSZMIN)
#define USTACKSZ        (STACKSZMIN)
#define STACKSZMAX      (KiB(512))
#define BADSTACKSZ(sz)  ((sz) < STACKSZMIN || (sz) > STACKSZMAX)

int builtin_threads_begin(usize  *nthreads);

#define thread_debugloc() ({                                                                 \
    printk(                                                                                  \
        "%s:%d in %s(), current[%d] ste: %s:%s @%s:%d "                                      \
        "by thread[%d], signal: %s, return[%p]\n",                                           \
        __FILE__, __LINE__, __func__,                                                        \
        current ? current->t_tid : 0, current ? current_killed() ? "killed" : "n/a" : "n/a", \
        current ? t_states[current->t_state] : "n/a",                                        \
        current ? current->t_statefile ? current->t_statefile : "no-file" : "no-file",       \
        current ? current->t_stateline : -1,                                                 \
        current ? current->t_statetid : 0,                                                   \
        current ? current_ishandling() "handling" : "n/a" : "n/a", __retaddr(0));            \
})

#define THREAD_CREATE_USER      BS(0) // create a user thread.
#define THREAD_CREATE_SCHED     BS(1) // create and schedule a thread.
#define THREAD_CREATE_DETACHED  BS(2) // create a detachable thread.
#define THREAD_CREATE_GROUP     BS(3) // create thread in new thread group.

int thread_sigqueue(thread_t *thread, siginfo_t *info);
int thread_sigdequeue(thread_t *thread, siginfo_t **ret);

int thread_detach(thread_t *thread);

/**
 * \brief schedule a thread.
 * \brief thread must be locked by caller.
 * \param thread to be scheduled.
 \return (int)0 on success of error on failure.
*/
int thread_schedule(thread_t *thread);

/**
 * \brief get the thread ID of thread.
 * \param thread whose ID we want.
 * \return tid of thread.
*/
tid_t thread_gettid(thread_t *thread);

/**
 * \brief Return the threadID of the current thread.
 * \return tid thread ID.
*/
tid_t thread_self(void);

/**
 * Yield current thread execution.
*/
void thread_yield(void);

/**
 * Kill all threads.
*/
int thread_kill_all(void);

/**
 * \brief Cancel the thread 'tid'.
 * \param thread thread to be cancelled.
 * \return (int)0 on success of error on failure.
*/
int thread_cancel(tid_t tid);

/**
 * Destroy 'thread'.
 * \param thread thread to be destroyed.
 *
*/
void thread_free(thread_t *thread);

/**
 * \brief Wake up a thread.
 * \param thread thread to be wake up.
 * \return (int)0 if successful or error on failure. 
 * \return (int)0 on success of error on failure.
*/
int thread_wake(thread_t *thread);

/**
 * \brief Kill thread 'tid'.
 * \param tid is id of the thread to be killed.
 * \param wait if '0' the function doen't wait for thread to die else it will wait.
 * \return (int)0 if successful or error on failure.
 */
int thread_kill(tid_t tid, int wait);

/**
 * \brief Exit current thread.
 * \param exit_code exit status of the current thread.
 */
void thread_exit(uintptr_t exit_code);

/**
 * \brief A thread from a thread queue.
 * \param queue thread queue.
*/
thread_t *thread_dequeue(queue_t *queue);

/**
 * @brief Allocate a kernel stack.
 * @param size 
 * @param ret 
 * @return 
 */
int thread_kstack_alloc(usize  size, uintptr_t *ret);

/**
 * \brief Deallocate the kernel thread stack.
 * \param addr base address of the kernel stack.
*/
void thread_kstack_free(uintptr_t addr);

/** \brief Kill thread.
 * \param thread is id of the thread to be killed.
 * \param wait if '0' the function doen't wait for thread to die else it will wait.
 * \return (int)0 if successful or error on failure. 
*/
int thread_kill_n(thread_t *thread, int wait);

/**
 * \brief Remove a thread from a thread queue.
 * \param thread thread to be removed.
 * \param queue thread queue.
 * \return (int)0 on success or error on faliure.
*/
int thread_remove_queue(thread_t *thread, queue_t *queue);

/**
 * \brief Wait for thread to terminate.
 * \param thread thread to be waited upon.
 * \param reap if '1' then the resources allocated to the thread and the structure are freed.
 * \param info pointer to minimal status info of thread.
 * \param retval return value of the thread that is being waited upon.
 * \return (int)0 on success or error on faliure.
*/
int thread_reap(thread_t *thread, int reap, thread_info_t *info, void **retval);

/**
 * \brief Wait for thread to terminate.
 * \param tid thread to be waited upon.
 * * if tid == 0, then any thread is joined provided it has terminated otherwise the function returns immediately.
 * \param info if non-null brief info about the state of the thread at termination is passed.
 * \param retval return value of the thread that is being joined.
 * \return (int)0 on success or error on faliure.
 */
int thread_join(tid_t tid, thread_info_t *info, void **retval);

/**
 * @brief allocate a new thread structure.
 * 
 * @param kstacksz size of the new thread's kernel stack.
 * @param flags flags used in thread creation, e.g THREAD_USER(if a user thread is intended).
 * @param ref the new structure is passed by reference through a pointer to a thread pointer.
 * @return int 0 on success otherwise an error code is passed.
 */
int thread_alloc(uintptr_t kstacksz, int flags, thread_t **ref);

/**
 * \brief Get a thread from a thread queue.
 * \param tid thread to be retrieved from queue.
 * if tid == 0, then any thread is returned.
 * \param pthread return the retrived thread through a pointer to the thread.
 * \return (int)0 on success or error on faliure.
 */
int thread_queue_get(queue_t *queue, tid_t tid, thread_t **pthread);

/**
 * \brief Create and join(wait for termination) a new kernel thread.
 * \param entry entry point of kernel thread.
 * \param arg argument to be passed to the new kernel thread
 * \param ret return value from the kernel thread after termination.
 * \return (int)0 on success or error on faliure.
 */
int kthread_create_join(void *(*entry)(void *), void *arg, void **ret);

/**
 * \brief Join and wait for thread to terminate.
 * \param thread thread to be joined.
 * \param info if non-null brief info about the state of the thread at termination is passed.
 * \param retval return value of the thread that is being joined.
 * \return (int)0 on success or error on faliure.
 */
int thread_join_r(thread_t *thread, thread_info_t *info, void **retval);

/**
 * \brief Put a thread onto a thread queue.
 * \param thread thread to be placed on the thread queue.
 * \param rnode pointer to pointer to queue node.
 * \return (int)0 on success or error on faliure.
 */
int thread_enqueue(queue_t *queue, thread_t *thread, queue_node_t **rnode);

/**
 * \brief Create a new kernel thread.
 * \param entry entry point of kernel thread.
 * \param arg argument to be passed to the new kernel thread.
 * \param __tid tid of the created kernel thread is passed(returned) via this pointer.
 * \param pthread pointer to a pointer to the newly created kernel thread is passed through pthread.
 * \return (int)0 on success or error on faliure.
 */
int kthread_create(thread_attr_t *__attr, thread_entry_t entry, void *arg, int flags, thread_t **ret);

/**
 * \brief Create a thread.
 * \param ptid thread ID of created thread is passed through ptid.
 * \param pthread pointer to the thread struct is passed by reference through pthread.
 * 
 * \param attr attributes used to specify how to create the new thread.
 * \param entry entry point of kernel thread.
 * \param arg argument to be passed to the new kernel thread.
 * \return (int)0 on success or error on faliure.
 */
int thread_create(thread_attr_t *attr, thread_entry_t entry, void *arg, thread_t **pthread);

int thread_sigmask(thread_t *thread, int how, const sigset_t *restrict set, sigset_t *restrict oset);

/// @brief 
/// @param thread 
/// @param queue 
/// @return 
int thread_stop(thread_t *thread, queue_t *queue);

int thread_execve(proc_t *proc, thread_t *thread,
    thread_entry_t entry, const char *argp[], const char *envp[]);

int thread_join_group(thread_t *thread);
int thread_leave_group(thread_t *thread);
int thread_create_group(thread_t *thread);
int thread_fork(thread_t *dst, thread_t *src, mmap_t *mmap);
int thread_get(tid_t tid, tstate_t state, thread_t **ppthread);