#include <arch/cpu.h>
#include <bits/errno.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <sys/thread.h>

int tgroup_destroy(tgroup_t *tgroup) {
    int err = 0;
    thread_t *thread = NULL;

    if (tgroup)
        return -EINVAL;

    tgroup->tg_signals = SIGNAL_INIT();
    
    tgroup_lock(tgroup);

    tgroup_kill_thread(tgroup, 0, 1);

    while (!tgroup_get_thread(tgroup, 0, 0, &thread)) {
        
    }

    tgroup_unlock(tgroup);

    return 0;
}

size_t tgroup_inc_running(tgroup_t *tgroup) {
    tgroup_assert_locked(tgroup);
    return ++tgroup->tg_running;
}

size_t tgroup_dec_running(tgroup_t *tgroup) {
    tgroup_assert_locked(tgroup);
    return --tgroup->tg_running;
}

size_t tgroup_thread_count(tgroup_t *tgroup) {
    size_t count = 0;
    tgroup_queue_lock(tgroup);
    count = queue_count(tgroup->tg_queue);
    tgroup_queue_unlock(tgroup);
    return count;
}

size_t tgroup_running_threads(tgroup_t *tgroup) {
    tgroup_assert_locked(tgroup);
    return tgroup->tg_running;
}

int tgroup_kill_thread(tgroup_t *tgroup, tid_t tid, int wait) {
    int err = 0;
    thread_t *thread = NULL;
    queue_node_t *next = NULL;
    tgroup_assert_locked(tgroup);

    if (current_killed())
        return -EINTR;

    if (tid == -1) {
        tgroup_queue_lock(tgroup);
        forlinked(node, tgroup->tg_queue->head, next) {
            next = node->next;
            thread = node->data;
            if (thread == current)
                continue;

            thread_lock(thread);
            tgroup_queue_unlock(tgroup);
            tgroup_unlock(tgroup);

            if ((err = thread_kill_n(thread, wait))) {
                thread_unlock(thread);
                goto error;
            }

            tgroup_lock(tgroup);
            tgroup_queue_lock(tgroup);
            thread_unlock(thread);
        }
        tgroup_queue_unlock(tgroup);
    } else {
        if ((err = tgroup_get_thread(tgroup, tid, 0, &thread)))
            goto error;

        thread_assert_locked(thread);

        if (thread == current) {
            current_unlock();
            thread_exit(0);
        }

        if ((err = thread_kill_n(thread, wait))) {
            thread_unlock(thread);
            goto error;
        }

        thread_unlock(thread);
    }

    return 0;
error:
    return err;
}

int tgroup_create(thread_t *tmain, tgroup_t **ptgroup) {
    int err = 0;
    tgroup_t *tgroup = NULL;
    queue_t *stopq = NULL, *queue = NULL;

    thread_assert_locked(tmain);

    if (tmain->t_group)
        return -EINVAL;

    if (!ptgroup)
        return -EINVAL;
    
    if (!(tgroup = kmalloc(sizeof *tgroup)))
        return -ENOMEM;


    if ((err = queue_new("tgroup-stop", &stopq)))
        goto error;
    
    if ((err = queue_new("tgroup", &queue)))
        goto error;

    memset(tgroup, 0, sizeof *tgroup);

    tgroup->tg_queue = queue;
    tgroup->tg_stopq = stopq;
    tgroup->tg_tmain = tmain;
    tgroup->tg_tlast = tmain;
    tgroup->tg_twait = tmain;
    tgroup->tg_tgid = tmain->t_tid;
    tgroup->tg_lock = SPINLOCK_INIT();

    tgroup_lock(tgroup);
    *ptgroup = tgroup;

    if ((err = tgroup_add_thread(tgroup, tmain)))
        goto error;

    return 0;
error:
    if (queue)
        queue_free(queue);
    if (stopq)
        queue_free(stopq);
    if (tgroup)
        kfree(tgroup);
    return err;
}

int tgroup_add_thread(tgroup_t *tgroup, thread_t *thread) {
    int err = 0;
    tgroup_assert_locked(tgroup);
    thread_assert_locked(thread);

    if (!thread)
        return -EINVAL;
    
    if ((err = thread_enqueue(tgroup->tg_queue, thread, NULL)))
        return err;
    
    thread->t_group = tgroup;
    return 0;
}

int tgroup_remove_thread(tgroup_t *tgroup, thread_t *thread) {
    int err = 0;
    tgroup_assert_locked(tgroup);
    thread_assert_locked(thread);

    if (!thread)
        return -EINVAL;

    queue_lock(tgroup->tg_queue);

    if ((err = thread_remove_queue(thread, tgroup->tg_queue))) {
        queue_unlock(tgroup->tg_queue);
        return err;
    }

    queue_unlock(tgroup->tg_queue);

    thread->t_group = NULL;
    return 0;
}

int tgroup_get_thread(tgroup_t *tgroup, tid_t tid, tstate_t state, thread_t **pthread) {
    thread_t *thread = NULL;
    queue_node_t *next = NULL;

    if (!pthread)
        return -EINVAL;

    tgroup_queue_lock(tgroup);

    forlinked(node, tgroup->tg_queue->head, next) {
        next = node->next;
        thread = node->data;

        thread_lock(thread);

        if (tid == 0 && thread_isstate(thread, state)) {
            tgroup_queue_unlock(tgroup);
            *pthread = thread;
            return 0;
        }

        if (thread->t_tid == tid || tid == -1) {
            tgroup_queue_unlock(tgroup);
            *pthread = thread;
            return 0;
        }

        thread_unlock(thread);
    }

    tgroup_queue_unlock(tgroup);

    return -ESRCH;
}