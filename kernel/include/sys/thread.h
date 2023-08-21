#pragma once

#include <arch/x86_64/pml4.h>
#include <arch/x86_64/thread.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <lib/types.h>
#include <sync/spinlock.h>
#include <sys/system.h>
#include <arch/cpu.h>
#include <ds/queue.h>
#include <bits/errno.h>
#include <sync/cond.h>
#include <sys/_time.h>
#include <ginger/jiffies.h>
#include <sys/_signal.h>

typedef enum tstate_t {
    T_EMBRYO,
    T_READY,
    T_RUNNING,
    T_ISLEEP,
    T_USLEEP,
    T_STOPPED,
    T_TERMINATED,
    T_ZOMBIE,
} tstate_t;

extern const char *t_states[];

typedef void *(*thread_entry_t)(void *);

typedef struct
{
    int detachstate;
    size_t guardsz;
    uintptr_t stackaddr;
    size_t stacksz;
} thread_attr_t;

typedef struct {
    time_t      ctime;      // Thread ceation time.
    time_t      cpu_time;   // CPU time in jiffies(n seconds = (jiffy * (HZ_TO_ns(SYS_HZ) / seconds_TO_ns(1))) ).
    time_t      timeslice;  // Quantum of CPU time for which this thread is allowed to run.
    time_t      total_time; // Total time this thread has run.
    time_t      last_sched; // Last time this thread was scheduled to run.

    cpu_t       *processor; // Processor for which this thread has affinity.
    atomic_t    affinity;   // Type of affinity (SOFT or HARD)
    atomic_t    priority;   // Thread scheduling Priority.
} sched_attr_t;

#define SCHED_ATTR_DEFAULT() (sched_attr_t){0}

/*soft affinity for the cpu*/
#define SCHED_SOFT_AFFINITY 0

/*hard affinity for the cpu*/
#define SCHED_HARD_AFFINITY 1

typedef struct {
    queue_t         *queue; // thread's sleep queue.
    queue_node_t    *node;  // thread's sleep node.
    spinlock_t      *guard; // non-null if sleep queue is associated with a guard lock.
} sleep_attr_t;

typedef struct {
    // file_t *table[NFILE];
    spinlock_t lock;
} file_table_t;

typedef struct {
    tid_t           tg_tgid;
    file_table_t    tg_file;
    uint64_t        tg_flags;
    thread_t        *tg_tmain;
    thread_t        *tg_tlast;
    queue_t         *tg_queue;
    queue_t         *tg_stopq;
    size_t          tg_running;

    sigset_t        sig_mask;
    sigaction_t     sig_action[NSIG];
    uint8_t         sig_queues[NSIG];
    spinlock_t      tg_lock;
} tgroup_t;

#define tgroup_assert(tg)               ({ assert(tg, "No thread group"); })
#define tgroup_lock(tg)                 ({ tgroup_assert(tg); spin_lock(&(tg)->tg_lock); })
#define tgroup_unlock(tg)               ({ tgroup_assert(tg); spin_unlock(&(tg)->tg_lock); })
#define tgroup_locked(tg)               ({ tgroup_assert(tg); spin_locked(&(tg)->tg_lock); })
#define tgroup_assert_locked(tg)        ({ tgroup_assert(tg); spin_assert_locked(&(tg)->tg_lock); })

#define tgroup_queue_assert(tg)         ({ tgroup_assert_locked(tg); queue_assert((tg)->tg_queue); })
#define tgroup_queue_lock(tg)           ({ tgroup_queue_assert(tg); queue_lock((tg)->tg_queue); })
#define tgroup_queue_unlock(tg)         ({ tgroup_queue_assert(tg); queue_unlock((tg)->tg_queue); })
#define tgroup_queue_locked(tg)         ({ tgroup_queue_assert(tg); queue_locked((tg)->tg_queue); })
#define tgroup_queue_assert_locked(tg)  ({ tgroup_queue_assert(tg); queue_assert_locked((tg)->tg_queue); })

/**
 * \brief Destroy this tgroup. But only after all threads are killed.
 * \param tgroup thread group.
 * \returns (int)0, on success and err on failure.
 **/
int tgroup_destroy(tgroup_t *tgroup);

/**
 * \brief No. of threads in this tgroup.
 * \brief Callers must hold tgroup->lock before calling into this function.
 * \param tgroup thread group.
 * \returns (int)0, on success and err on failure.
 **/
size_t tgroup_thread_count(tgroup_t *tgroup);

/**
 * \brief No. of threads running in this tgroup.
 * \brief Callers must hold tgroup->lock before calling into this function.
 * \param tgroup thread group.
 * \returns (int)0, on success and err on failure.
 **/
size_t tgroup_running_threads(tgroup_t *tgroup);

/**
 * \brief Increase running thread count in this tgroup.
 * \brief Callers must hold tgroup->lock before calling into this function.
 **/
size_t tgroup_inc_running(tgroup_t *tgroup);

/**
 * \brief Decrease running thread count in this tgroup.
 * \brief Callers must hold tgroup->lock before calling into this function.
 **/
size_t tgroup_dec_running(tgroup_t *tgroup);

/**
 * \brief Kill a thread in this tgroup.
 * \brief Callers must hold tgroup->lock before calling into this function.
 * \param tgroup thread group.
 * \param tid
 *  Is the absolute threadID of thread to kill, if tid == -1, then kills all threads.
 * However, if 'current' is in this group and tid == -1, then all thread except 'current' will be killed.
 * \param wait wait for thread to die?
 * \returns (int)0, on success and err on failure.
 **/
int tgroup_kill_thread(tgroup_t *tgroup, tid_t tid, int wait);

/**
 * \brief Create a new thread tgroup.
 * \param ptgroup thread group reference pointer.
 * \returns (int)0, on success and err on failure.
 **/
int tgroup_create(tgroup_t **ptgroup);

/**
 * \brief Add a thread to this tgroup.
 * \brief Callers must hold tgroup->lock and thread->t_lock before calling into this function.
 * \param tgroup thread group.
 * \param thread thread to be added thread group.
 * \returns (int)0, on success and err on failure.
 **/
int tgroup_add_thread(tgroup_t *tgroup, thread_t *thread);

/**
 * \brief Remove a thread to this tgroup.
 * \brief Callers must hold tgroup->lock and thread->t_lock before calling into this function.
 * \param tgroup thread group.
 * \param thread thread to be removed thread group.
 * \returns (int)0, on success and err on failure.
 **/
int tgroup_remove_thread(tgroup_t *tgroup, thread_t *thread);

/**
 * \brief Get a thread belonging to this tgroup.
 * \brief Callers must hold tgroup->lock before calling into this function.
 * \param tgroup thread group.
 * \param tid absolute threadID of thread to return.
 * \param state if tid == 0, return any thread matching 'state'.
 * if tid == -1 then return any thread.
 * \param pthread reference pointer to returned thread.
 * \returns (int)0, on success and err on failure.
 **/
int tgroup_get_thread(tgroup_t *tgroup, tid_t tid, tstate_t state, thread_t **pthread);

/// @brief 
/// @param tgroup 
/// @param signo 
/// @return 
int tgroup_sigqueue(tgroup_t *tgroup, int signo);


/// @brief 
/// @param tgroup 
/// @param how 
/// @param set 
/// @param oset 
/// @return 
int tgroup_sigprocmask(tgroup_t *tgroup, int how, const sigset_t *restrict set, sigset_t *restrict oset);

int tgroup_stop(tgroup_t *tgroup);

int tgroup_terminate(tgroup_t *tgroup, spinlock_t *lock);

int tgroup_spawn(thread_entry_t entry, void *arg, int flags, tgroup_t **ptgroup);

int tgroup_thread_create(tgroup_t *tgroup, thread_entry_t entry, void *arg, int flags, int sched, thread_t **pthread);

typedef struct thread {
    tid_t           t_tid;              // thread ID.
    tid_t           t_killer;           // thread that killed this thread.
    uintptr_t       t_entry;            // thread entry point.
    tstate_t        t_state;            // thread's execution state.
    tid_t           t_statetid;
    char            *t_statefile;
    long            t_stateline;
    uintptr_t       t_exit;             // thread exit code.
    atomic_t        t_flags;            // thread's flags.
    atomic_t        t_spinlocks;
    uintptr_t       t_errno;            // thread's errno.
    
    thread_attr_t   t_attr;             // thread' attributes.

    sigset_t        t_sigmask;          // thread's masked signal set.
    uint8_t         t_sigqueue[NSIG];   // thread's queue of pending signals.

    void            *t_simd_ctx;        // thread's Single Instruction Multiple Data (SIMD) context.
    pagemap_t       *t_map;             // thread's process virtual address space.
    sleep_attr_t    sleep_attr;         // struct describing sleep attributes for this thread.
    sched_attr_t    t_sched_attr;       // struct describing scheduler attributes for this thread.

    tgroup_t        *t_group;           // thread group.
    queue_t         *t_queues;          // queues on which this thread resides.

    cond_t          *t_wait;            // thread conditional wait variable.
    x86_64_thread_t t_arch;             // architecture thread struct.

    spinlock_t      t_lock;             // lock to synchronize access to this struct.
} __aligned(16) thread_t;

typedef struct {
    tid_t           ti_tid;
    tid_t           ti_killer;
    tstate_t        ti_state;
    sched_attr_t    ti_sched;
    uintptr_t       ti_errno;
    uintptr_t       ti_exit;
    atomic_t        ti_flags;
} thread_info_t;

#define THREAD_USER                     BS(0)   // thread is a user thread.
#define THREAD_KILLED                   BS(1)   // thread was killed by another thread.
#define THREAD_SETPARK                  BS(2)   // thread has the park flag set.
#define THREAD_SETWAKE                  BS(3)   // thread has the wakeup flag set.
#define THREAD_HANDLING_SIG             BS(4)   // thread is currently handling a signal.
#define THREAD_DETACHED                 BS(5)   // free resources allocated to this thread imediately to terminates.
#define THREAD_STOP                     BS(6)   // thread stop
#define THREAD_SIMD_DIRTY               BS(7)   // thread's SIMD context is dirty and must be save on context swtich.

#define thread_assert(t)                ({ assert(t, "No thread pointer\n");})
#define thread_lock(t)                  ({ thread_assert(t); spin_lock(&((t)->t_lock)); })
#define thread_unlock(t)                ({ thread_assert(t); spin_unlock(&((t)->t_lock)); })
#define thread_locked(t)                ({ thread_assert(t); spin_locked(&((t)->t_lock)); })
#define thread_assert_locked(t)         ({ thread_assert(t); spin_assert_locked(&((t)->t_lock)); })

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

#define thread_killed(t) ({                            \
    int locked = thread_locked(t);                     \
    if (!locked)                                       \
        thread_lock(t);                                \
    int killed = thread_testflags((t), THREAD_KILLED); \
    if (!locked)                                       \
        thread_unlock(t);                              \
    killed;                                            \
})

#define thread_ishandling_signal(t) ({                         \
    int locked = thread_locked(t);                             \
    if (!locked)                                               \
        thread_lock(t);                                        \
    int handling = thread_testflags((t), THREAD_HANDLING_SIG); \
    if (!locked)                                               \
        thread_unlock(t);                                      \
    handling;                                                  \
})

#define thread_tgroup(t)                ({ thread_assert(t); (t)->t_group; })
#define thread_tgroup_lock(t)           ({ tgroup_lock(thread_tgroup(t)); })
#define thread_tgroup_unlock(t)         ({ tgroup_unlock(thread_tgroup(t)); })
#define thread_tgroup_locked(t)         ({ tgroup_locked(thread_tgroup(t)); })

#define thread_isuser(t)                ({ thread_testflags((t), THREAD_USER); })
#define thread_isdetached(t)            ({ thread_testflags((t), THREAD_DETACHED); })
#define thread_issetwake(t)             ({ thread_testflags((t), THREAD_SETWAKE); })
#define thread_issetpark(t)             ({ thread_testflags((t), THREAD_SETPARK); })

#define thread_setuser(t)               ({ thread_setflags((t), THREAD_USER); })
#define thread_setdetached(t)           ({ thread_setflags((t), THREAD_DETACHED); })
#define thread_setwake(t)               ({ thread_setflags((t), THREAD_SETWAKE); })
#define thread_setpark(t)               ({ thread_setflags((t), THREAD_SETPARK); })
#define thread_set_park_wake(t)         ({ thread_setflags((t), THREAD_SETWAKE | THREAD_SETPARK); })

#define thread_maskdetached(t)          ({ thread_maskflags((t), THREAD_DETACHED); })
#define thread_maskwake(t)              ({ thread_maskflags((t), THREAD_SETWAKE); })
#define thread_maskpark(t)              ({ thread_maskflags((t), THREAD_SETPARK); })
#define thread_mask_park_wake(t)        ({ thread_maskflags((t), THREAD_SETWAKE | THREAD_SETPARK); })

#define thread_issetpark_wake(t)        ({ thread_testflags((t), THREAD_SETWAKE | THREAD_SETPARK); })
#define thread_issimd_dirty(t)          ({ thread_testflags((t), THREAD_SIMD_DIRTY); })
#define thread_set_simd_dirty(t)        ({ thread_setflags((t), THREAD_SIMD_DIRTY); })
#define thread_mask_simd_dirty(t)       ({ thread_maskflags((t), THREAD_SIMD_DIRTY); })

#define current_assert()                ({ assert(current, "No current thread running"); })
#define current_lock()                  ({ thread_lock(current); })
#define current_unlock()                ({ thread_unlock(current); })
#define current_locked()                ({ thread_locked(current); })
#define current_assert_locked()         ({ thread_assert_locked(current); })

#define current_tgroup()                ({ thread_tgroup(current); })
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

#define current_isuser()                ({ thread_isuser(current); })
#define current_killed()                ({ thread_killed(current); })
#define current_issetwake()             ({ thread_issetwake(current); })
#define current_issetpark()             ({ thread_issetpark(current); })
#define current_issetpark_wake()        ({ thread_issetpark_wake(current); })
#define current_isdetached()            ({ thread_isdetached(current); })
#define current_handling()              ({ thread_ishandling_signal(current); })
#define current_isstate(state)          ({ thread_isstate(current, state); })
#define current_embryo()                ({ thread_isembryo(current); })
#define current_ready ()                ({ thread_isready(current); })
#define current_isisleep()              ({ thread_isisleep(current); })
#define current_isusleep()              ({ thread_isusleep(current); })
#define current_isrunning()             ({ thread_isrunning(current); })
#define current_isstopped()             ({ thread_isstopped(current); })
#define current_iszombie()              ({ thread_iszombie(current); })
#define current_isterminated()          ({ thread_isterminated(current); })
#define current_setflags(flags)         ({ thread_setflags(current, (flags)); })
#define current_testflags(flags)        ({ thread_testflags(current, (flags)); })
#define current_maskflags(flags)        ({ thread_maskflags(current, (flags)); })
#define current_enter_state(state)      ({ thread_enter_state(current, (state)); })

#define current_issimd_dirty()          ({ thread_issimd_dirty(current); })
#define current_set_simd_dirty()        ({ thread_set_simd_dirty(current); })
#define current_mask_simd_dirty()       ({ thread_mask_simd_dirty(current); })

#define BUILTIN_THREAD(name, entry, arg)                                                        \
    __section(".__builtin_thread_entry")    void *__CAT(__builtin_thread_entry_, name) = entry; \
    __section(".__builtin_thread_arg")      void *__CAT(__builtin_thread_arg_, name) = (void *)arg;

#define BUILTIN_THREAD_ANOUNCE(name)    ({ printk("\"%s\" thread [tid: %d] running...\n", name, thread_self()); })

#define STACKSZMIN      (16 * KiB)
#define KSTACKSZ        (STACKSZMIN)
#define STACKSZMAX      (512 * KiB)
#define BADSTACKSZ(sz)  ((sz) < STACKSZMIN || (sz) > STACKSZMAX)

int builtin_threads_begin(int *nthreads, thread_t ***threads);

#define thread_debugloc() ({                                                                                                       \
    printk("%s:%d in %s(), current[%d] state: %s:%s @%s:%d by thread[%d], signal: %s, return[%p]\n",                                                     \
           __FILE__, __LINE__, __func__, current ? current->t_tid : 0, current ? current_killed() ? "killed" : "n/a" : "n/a",      \
           current ? t_states[current->t_state] : "n/a", current ? current->t_statefile ? current->t_statefile : "no-file" : "no-file",\
           current ? current->t_stateline : -1, \
           current ? current->t_statetid : 0,\
           current ? current_handling() ? "handling" : "n/a" : "n/a", __retaddr(0)); \
})

int thread_sigdequeue(thread_t *thread);
int thread_sigqueue(thread_t *thread, int signo);

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
 * \brief Allocate a kernel stack
 * \param size size of the kernel stack to allocate.
*/
uintptr_t thread_alloc_kstack(size_t size);

/** \brief Kill thread.
 * \param thread is id of the thread to be killed.
 * \param wait if '0' the function doen't wait for thread to die else it will wait.
 * \return (int)0 if successful or error on failure. 
*/
int thread_kill_n(thread_t *thread, int wait);

/**
 * \brief Deallocate the kernel thread stack.
 * \param addr base address of the kernel stack.
 * \param size size of the kernel stack.
*/
void thread_free_kstack(uintptr_t addr, size_t size);

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
 * \brief Allocate a new thread structure.
 * \param attr attributes used to specify how to create the new thread.
 * \param entry entry point of kernel thread.
 * \param arg argument to be passed to the new kernel thread
 * \param flags additional specs for thread creation.
 * \param pthread return the newly allocated thread through a pointer to the thread.
 * \return (int)0 on success or error on faliure.
 */
int thread_new(thread_attr_t *attr, thread_entry_t entry, void *arg, int flags,  thread_t **pthread);

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
int kthread_create(thread_entry_t entry, void *arg, tid_t *__tid, thread_t **pthread);

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
int thread_create(thread_t **pthread, thread_attr_t *attr, thread_entry_t entry, void *arg);

int thread_sigmask(thread_t *thread, int how, const sigset_t *restrict set, sigset_t *restrict oset);

/// @brief 
/// @param thread 
/// @param queue 
/// @return 
int thread_stop(thread_t *thread, queue_t *queue);