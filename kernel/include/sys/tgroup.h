#pragma once

#include <ds/queue.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <sync/spinlock.h>
#include <lib/types.h>

typedef struct __thread_t thread_t;

#define tgroup_assert(tgroup)               ({ assert(tgroup, "No tgroup."); })
#define tgroup_lock(tgroup)                 ({ tgroup_assert(tgroup); queue_lock(tgroup); })
#define tgroup_unlock(tgroup)               ({ tgroup_assert(tgroup); queue_unlock(tgroup); })
#define tgroup_islocked(tgroup)             ({ tgroup_assert(tgroup); queue_islocked(tgroup); })
#define tgroup_assert_locked(tgroup)        ({ tgroup_assert(tgroup); queue_assert_locked(tgroup); })

#define tgroup_getid(tgroup)                ({ tgroup_assert_locked(tgroup); (tgroup)->tg_tgid; })

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
void tgroup_destroy(queue_t *tgroup);

/**
 * \brief Kill a thread in this tgroup.
 * \brief Callers must hold tgroup->lock before calling into this function.
 * \param tgroup thread group.
 * \param tid
 *  Is the absolute threadID of thread to kill, if tid == -1, then kills all threads.
 * However, if 'current' is in this group and tid == -1, then all thread except 'current' will be killed
 * and 'current' becomes the main thread of this tgroup.
 * Further, if except_flags are set, any thread with a flags bit set in except_flags will be skipped.
 * \param wait wait for thread to die?
 * \returns (int)0, on success and err on failure.
 **/
int tgroup_kill_thread(queue_t *tgroup, tid_t tid, i32 except_flags, int wait);

/**
 * \brief Create a new thread tgroup.
 * \param ptgroup thread group reference pointer.
 * \returns (int)0, on success and err on failure.
 **/
int tgroup_create(queue_t **ptgroup);

/**
 * \brief Add a thread to this tgroup.
 * \brief Callers must hold tgroup->lock and thread->t_lock before calling into this function.
 * \param tgroup thread group.
 * \param thread thread to be added thread group.
 * \returns (int)0, on success and err on failure.
 **/
int tgroup_add_thread(queue_t *tgroup, thread_t *thread);

/**
 * \brief Remove a thread to this tgroup.
 * \brief Callers must hold tgroup->lock and thread->t_lock before calling into this function.
 * \param tgroup thread group.
 * \param thread thread to be removed thread group.
 * \returns (int)0, on success and err on failure.
 **/
int tgroup_remove_thread(queue_t *tgroup, thread_t *thread);

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
int tgroup_get_thread(queue_t *tgroup, tid_t tid, tstate_t state, thread_t **pthread);

int tgroup_stop(queue_t *tgroup);
int tgroup_continue(queue_t *tgroup);
int tgroup_terminate(queue_t *tgroup, spinlock_t *lock);
int tgroup_getmain(queue_t *tgroup, thread_t **ptp);

int tgroup_suspend(queue_t *tgroup);
int tgroup_unsuspend(queue_t *tgroup);