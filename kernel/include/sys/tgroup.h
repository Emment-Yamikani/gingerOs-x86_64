#pragma once

#include <ds/queue.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <sync/spinlock.h>
#include <lib/types.h>

struct thread;
typedef struct thread thread_t;

typedef struct {
    tid_t           tg_tgid;

    thread_t        *tg_tmain;
    thread_t        *tg_tlast;

    long            tg_refcnt;
    long            tg_running;
    queue_t         tg_thread;
    file_table_t    tg_file_table;

    sigset_t        sig_mask;
    sigaction_t     sig_action[NSIG];
    uint8_t         sig_queues[NSIG];

    spinlock_t  tg_lock;
} tgroup_t;

#define tgroup_assert(tgroup)               ({ assert(tgroup, "No tgroup."); })
#define tgroup_lock(tgroup)                 ({ tgroup_assert(tgroup); spin_lock(&(tgroup)->tg_lock); })
#define tgroup_unlock(tgroup)               ({ tgroup_assert(tgroup); spin_unlock(&(tgroup)->tg_lock); })
#define tgroup_islocked(tgroup)             ({ tgroup_assert(tgroup); spin_islocked(&(tgroup)->tg_lock); })
#define tgroup_assert_locked(tgroup)        ({ tgroup_assert(tgroup); spin_assert_locked(&(tgroup)->tg_lock); })

#define tgroup_getid(tgroup)                ({ tgroup_assert_locked(tgroup); (tgroup)->tg_tgid; })

#define tgroup_getmain(tgroup)              ({ tgroup_assert_locked(tgroup); (tgroup)->tg_tmain; })

/**
 * \brief No. of threads running in this tgroup.
 * \brief Callers must hold tgroup->lock before calling into this function.
 * \param tgroup thread group.
 * \returns (int)0, on success and err on failure.
 **/
#define tgroup_running(tgroup)              ({ tgroup_assert_locked(tgroup); (tgroup)->tg_running; })

/**
 * \brief Increase running thread count in this tgroup.
 * \brief Callers must hold tgroup->lock before calling into this function.
 **/
#define tgroup_inc_running(tgroup)         ({ tgroup_assert_locked(tgroup); (tgroup)->tg_running++; })

/**
 * \brief Decrease running thread count in this tgroup.
 * \brief Callers must hold tgroup->lock before calling into this function.
 **/
#define tgroup_dec_running(tgroup)         ({ tgroup_assert_locked(tgroup); (tgroup)->tg_running--; })

#define tgroup_getref(tgroup)               ({ tgroup_assert_locked(tgroup); (tgroup)->tg_refcnt++; })
#define tgroup_putref(tgroup)               ({ tgroup_assert_locked(tgroup); (tgroup)->tg_refcnt--; })
#define tgroup_release(tgroup)              ({ tgroup_putref(tgroup); tgroup_unlock(tgroup); })

#define tgroup_queue(tgroup)                ({ tgroup_assert_locked(tgroup); &(tgroup)->tg_thread; })
#define tgroup_queue_lock(tgroup)           ({ queue_lock(tgroup_queue(tgroup)); })
#define tgroup_queue_unlock(tgroup)         ({ queue_unlock(tgroup_queue(tgroup)); })
#define tgroup_queue_islocked(tgroup)       ({ queue_islocked(tgroup_queue(tgroup)); })
#define tgroup_queue_assert_locked(tgroup)  ({ queue_assert_locked(tgroup_queue(tgroup)); })

/**
 * \brief No. of threads in this tgroup.
 * \brief Callers must hold tgroup->lock before calling into this function.
 * \param tgroup thread group.
 * \returns (int)0, on success and err on failure.
 **/
#define tgroup_getthread_count(tgroup) ({           \
    tgroup_queue_lock(tgroup);                      \
    size_t cnt = queue_count(tgroup_queue(tgroup)); \
    tgroup_queue_unlock(tgroup);                    \
    cnt;                                            \
})

/**
 * \brief Destroy this tgroup. But only after all threads are killed.
 * \param tgroup thread group.
 * \returns void.
 **/
void tgroup_destroy(tgroup_t *tgroup);

/**
 * \brief Kill a thread in this tgroup.
 * \brief Callers must hold tgroup->lock before calling into this function.
 * \param tgroup thread group.
 * \param tid
 *  Is the absolute threadID of thread to kill, if tid == -1, then kills all threads.
 * However, if 'current' is in this group and tid == -1, then all thread except 'current' will be killed
 * and 'current' becomes the main thread of this tgroup.
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
int tgroup_continue(tgroup_t *tgroup);

int tgroup_terminate(tgroup_t *tgroup, spinlock_t *lock);

int tgroup_spawn(thread_entry_t entry, void *arg, int flags, tgroup_t **ptgroup);

int tgroup_thread_create(tgroup_t *tgroup, thread_entry_t entry, void *arg, int flags, thread_t **pthread);