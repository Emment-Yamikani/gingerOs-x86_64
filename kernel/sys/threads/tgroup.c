#include <arch/cpu.h>
#include <bits/errno.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <sys/thread.h>
#include <fs/fs.h>


int tgroup_kill_thread(queue_t *tgroup, tid_t tid, i32 except_flags, int wait) {
    int err = 0;

    tgroup_assert_locked(tgroup);

    if ((tid == -1)) {
        queue_foreach(thread_t *, thread, tgroup) {
            if (thread == current)
                continue;

            thread_lock(thread);
            if (thread_testflags(thread, except_flags)) {
                thread_unlock(thread);
                continue;
            }

            tgroup_unlock(tgroup);

            if ((err = thread_kill_n(thread, wait))) {
                thread_unlock(thread);
                goto error;
            }

            tgroup_lock(tgroup);
        }

        if (current_tgroup() == tgroup)
            thread_setmain(current);
    } else if (tid == -2){

    } else {

    }

    return 0;
error:
    return err;
}


int thread_create_group(thread_t *thread) {
    int         err         = 0;
    cred_t      *cred       = NULL;
    file_ctx_t  *fctx       = NULL;
    sig_desc_t  *sigdesc    = NULL;
    queue_t     *threads    = NULL;

    if (thread == NULL)
        return -EINVAL;

    thread_assert_locked(thread);

    if ((err = cred_alloc(&cred)))
        return err;

    if ((err = fctx_alloc(&fctx)))
        goto error;

    if ((err = sig_desc_alloc(&sigdesc)))
        goto error;

    if ((err = queue_alloc(&threads)))
        goto error;

    if ((err = thread_enqueue(threads, thread, NULL)))
        goto error;

    thread_setmain(thread);
    thread->t_cred      = cred;
    thread->t_fctx      = fctx;
    thread->t_tgroup    = threads;
    thread->t_sigdesc   = sigdesc;
    thread->t_tgid      = thread->t_tid;

    return 0;
error:
    if (threads)
        queue_free(threads);
    
    if (sigdesc)
        sig_desc_free(sigdesc);
    
    if (fctx)
        fctx_free(fctx);
    
    if (cred)
        cred_free(cred);
    
    return err;
}

int thread_join_group(thread_t *thread) {
    int         err   = 0;
    if ((err = thread_enqueue(current->t_tgroup,     thread, NULL)))
        return err;

    thread->t_cred      = current->t_cred;
    thread->t_fctx      = current->t_fctx;
    thread->t_tgid      = current->t_tgid;
    thread->t_owner     = current->t_owner;
    thread->t_tgroup    = current->t_tgroup;
    thread->t_sigdesc   = current->t_sigdesc;

    return 0;
}

int thread_leave_group(thread_t *thread) {
    int err = 0;
    thread_assert_locked(thread);

    queue_lock(thread->t_tgroup);
    thread_lock(thread);
    err = thread_remove_queue(thread, thread->t_tgroup);

    thread->t_cred          = NULL;
    thread->t_fctx          = NULL;
    thread->t_tgroup        = NULL;
    thread->t_sigdesc       = NULL;

    thread_unlock(thread);
    queue_unlock(thread->t_tgroup);
    assert_msg(err == 0, "tid: %d, Failed to leave group\n", thread_gettid(thread));
    return 0;
}

int tgroup_get_thread(queue_t *tgroup, tid_t tid, tstate_t state, thread_t **pthread) {
    thread_t        *thread = NULL;
    queue_node_t    *next   = NULL;

    if (!pthread)
        return -EINVAL;
    queue_assert_locked(tgroup);

    forlinked(node, tgroup->head, next) {
        next   = node->next;
        thread = node->data;

        thread_lock(thread);
        if (tid == 0 && thread_isstate(thread, state)) {
            *pthread = thread_getref(thread);
            return 0;
        }

        if (thread->t_tid == tid || tid == -1) {
            *pthread = thread_getref(thread);
            return 0;
        }
        thread_unlock(thread);
    }
    return -ESRCH;
}

int tgroup_terminate(queue_t *tgroup, spinlock_t *lock) {
    int     err = 0; 
    tgroup_assert_locked(tgroup);
    
    if (lock)
        spin_unlock(lock);

    if ((err = tgroup_kill_thread(tgroup, -1, 0, 0)))
        printk("ERROR OCCURED: %d\n", err);
    if (current_tgroup() == tgroup) {
        tgroup_unlock(tgroup);
        thread_exit(-EINTR);
    }
    return err;
}

int tgroup_continue(queue_t *tgroup) {
    tgroup_assert_locked(tgroup);
    queue_foreach(thread_t *, thread, tgroup) {
        thread_lock(thread);
        thread_wake(thread);
        thread_unlock(thread);
    }
    return 0;
}

int tgroup_stop(queue_t *tgroup) {
    tgroup_assert_locked(tgroup);
    queue_foreach(thread_t *, thread, tgroup) {
        thread_lock(thread);
        thread_setflags(thread, THREAD_STOP);
        thread_wake(thread);
        thread_unlock(thread);
    }
    return 0;
}

int tgroup_getmain(queue_t *tgroup, thread_t **ptp) {
    if (tgroup == NULL || ptp == NULL)
        return -EINVAL;
    
    tgroup_assert_locked(tgroup);
    queue_foreach(thread_t *, thread, tgroup) {
        thread_lock(thread);
        if (thread_ismain(thread)) {
            *ptp = thread_getref(thread);
            return 0;
        }
        thread_unlock(thread);
    }
    return -ESRCH;
}