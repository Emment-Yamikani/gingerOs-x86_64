#include <arch/cpu.h>
#include <bits/errno.h>
#include <sys/_signal.h>
#include <mm/kalloc.h>
#include <sys/sysproc.h>
#include <arch/thread.h>
#include <arch/signal.h>
#include <sys/proc.h>
#include <sys/thread.h>

/**
 * @brief checks if sender has permission
 * to send proc a signal.
 * 
 * @param proc proc is receiptient of signal.
 * @param ppuid pointer to uid of the sending process.
 * @return int 
 */
int signal_check_perm(proc_t *proc, uid_t *ppuid) {
    uid_t uid   = 0;
    uid_t gid   = 0;
    uid_t euid  = 0;
    uid_t egid  = 0;
    int granted = 0;

    if (proc == NULL)
        return -EPERM;

    proc_assert_locked(proc);

    // this is a call from the bare kernel.
    if (current == NULL) {
        if (ppuid)
            *ppuid = 0;
        return 0;
    }

    cred_lock(current->t_cred);
    uid = current->t_cred->c_uid;

    if (curproc == proc) {
        cred_unlock(current->t_cred);
        if (ppuid)
            *ppuid = uid;
        return 0;
    }

    gid  = current->t_cred->c_gid;
    euid = current->t_cred->c_euid;
    egid = current->t_cred->c_egid;
    cred_unlock(current->t_cred);

    if (!getpid()   || (uid == 0)  || (euid == 0) ||
        (gid == 0)  || (egid == 0)) {
        if (ppuid)
            *ppuid = uid;
        return 0;
    }

    cred_lock(proc->cred);
    granted = ((proc->cred->c_uid == uid)   ||
               (proc->cred->c_gid == gid)   ||
               (proc->cred->c_euid == euid) ||
               (proc->cred->c_egid == egid));
    cred_unlock(proc->cred);

    if (!granted)
        return -EPERM;

    if (ppuid)
        *ppuid = uid;
    return 0;
}

static int signal_select_thread(proc_t *proc, int signo, thread_t **pthread) {
    int         err     = 0;

    if (proc == NULL || pthread == NULL)
        return -EINVAL;

    queue_lock(proc->threads);
    /**
     * @brief try picking a thread
     * that is in an interruptable sleep state
     * and has the signo unblocked.
     */
    queue_foreach(thread_t *, thread, proc->threads) {
        if (current == thread)
            continue;

        thread_lock(thread);
        if (thread_isterminated(thread) ||
            thread_iszombie(thread) || 
            thread_isusleep(thread)) {
            thread_unlock(thread);
            continue;
        }
        // printk("%s:%d: thread(%d): state: %s\n", __FILE__, __LINE__, thread->t_tid, t_states[thread_getstate(thread)]);
        /**
         * @brief thread is not in the supported interruptable
         * blocked state. so just break the loop because
         * it is unlikely that the subsequent
         * threads are in a sleep state.
         * and if signo is not blocked return this thread.
         */
        if (thread_isstopped(thread) || thread_isisleep(thread)) {
            if (!sigismember(&thread->t_sigmask, signo)) {
                // put thread at back of queue.
                if ((err = queue_rellocate_node(proc->threads,
                                thread->t_tgrp_qn, QUEUE_RELLOC_TAIL))) {
                    thread_unlock(thread);
                    queue_unlock(proc->threads);
                    return err;
                }
                *pthread = thread;
                queue_unlock(proc->threads);
                return 0;
            }
        }

        thread_unlock(thread);
    }

    /**
     * @brief try any thread which is not currently handling a signal
     * and has the signo unblocked.
     */
    queue_foreach(thread_t *, thread, proc->threads) {
        if (current == thread)
            continue;

        thread_lock(thread);       
        if (thread_isterminated(thread) ||
            thread_iszombie(thread) || 
            thread_isusleep(thread)) {
            thread_unlock(thread);
            continue;
        }

        if (thread_isisleep(thread) || thread_isstopped(thread)) {
            thread_unlock(thread);
            continue;
        }

        // printk("%s:%d: thread(%d): state: %s\n", __FILE__, __LINE__, thread->t_tid, t_states[thread_getstate(thread)]);
        // only threads not currently handling signals.
        if (!thread_ishandling_signal(thread)) {
            // if signo is not blocked return this thread.
            if (!sigismember(&thread->t_sigmask, signo)) {
                // put thread at back of queue.
                if ((err = queue_rellocate_node(proc->threads,
                                thread->t_tgrp_qn, QUEUE_RELLOC_TAIL))) {
                    thread_unlock(thread);
                    queue_unlock(proc->threads);
                    return err;
                }
                *pthread = thread;
                queue_unlock(proc->threads);
                return 0;
            }
        }
        thread_unlock(thread);
    }

    // Try any thread which has the signal unblocked.
    queue_foreach(thread_t *, thread, proc->threads) {
        if (current == thread)
            continue;
        
        thread_lock(thread);       
        if (thread_isterminated(thread) ||
            thread_iszombie(thread) || 
            thread_isusleep(thread)) {
            thread_unlock(thread);
            continue;
        }

        if (thread_isisleep(thread) || thread_isstopped(thread)) {
            thread_unlock(thread);
            continue;
        }

        // skip threads currently handling signals.
        if (thread_ishandling_signal(thread)) {
            thread_unlock(thread);
            continue;
        }

        // if signo is not blocked return this thread.
        if (!sigismember(&thread->t_sigmask, signo)) {
            // put thread at back of queue.
            if ((err = queue_rellocate_node(proc->threads,
                            thread->t_tgrp_qn, QUEUE_RELLOC_TAIL))) {
                thread_unlock(thread);
                queue_unlock(proc->threads);
                return err;
            }
            *pthread = thread;
            queue_unlock(proc->threads);
            return 0;
        }

        thread_unlock(thread);
    }
    queue_unlock(proc->threads);

    return -ESRCH;
}

int signal_send(proc_t *proc, int signo) {
    int         err         = 0;
    uid_t       uid         = 0;
    siginfo_t   *info       = NULL;
    thread_t    *thread     = NULL;
    sig_desc_t  *sigdesc    = NULL;

    if (signo < 0 || signo > NSIG)
        return -EINVAL;

    proc_assert_locked(proc);

    if ((err = signal_check_perm(proc, &uid)))
        return err;
    else if (signo == 0)
        return 0; // Test Ok
    
    if (NULL == (info = (siginfo_t *)kmalloc(sizeof *info)))
        return -ENOMEM;
    
    info->si_code   = 0;
    info->si_status = 0;
    info->si_uid    = uid;
    info->si_addr   = NULL;
    info->si_signo  = signo;
    info->si_value  = (union sigval) { .sigval_int = signo };
    info->si_pid    = proc != curproc ? getpid() : curproc->pid;

    err = signal_select_thread(proc, signo, &thread);

    assert_msg(err == 0 || err == -ESRCH,
        "Error finding thread to signal. error: %d.\n", err
    );

    switch (err) {
    case 0:
        if ((err = thread_sigqueue(thread, info))) {
            thread_unlock(thread);
            kfree(info);
            return err;
        }
        thread_unlock(thread);
        break;
    case -ESRCH:
        sigdesc = proc->sigdesc;
        sigdesc_lock(sigdesc);
        queue_lock(&sigdesc->sig_queue[signo - 1]);
        if ((err = sigenqueue_pending(
            &sigdesc->sig_queue[signo - 1], info))) {
            queue_unlock(&sigdesc->sig_queue[signo - 1]);
            sigdesc_unlock(sigdesc);
            kfree(info);
            return err;
        }
        queue_unlock(&sigdesc->sig_queue[signo - 1]);
        sigdesc_unlock(sigdesc);
    }

    return 0;
}

int kill(pid_t pid, int signo) {
    int     err     = 0;
    pid_t   pgid    = 0;
    proc_t  *proc   = NULL;
    
    if (pid > 0) {
        if ((err = procQ_search_bypid(pid, &proc)))
            return err;
        
        if ((err = signal_send(proc, signo))) {
            proc_unlock(proc);
            return err;
        }

        proc_unlock(proc);
    } else if (pid == 0) {
        pgid = getpgrp();

        queue_lock(procQ);
        queue_foreach(proc_t *, proc, procQ) {
            if (proc == curproc)
                continue;

            proc_lock(proc);
            if (proc->pgid == pgid) {
                if ((err = signal_send(proc, signo))) {
                    proc_unlock(proc);
                    queue_unlock(procQ);
                    return err;
                }
            }
            proc_unlock(proc);
        }
        queue_unlock(procQ);

        proc_lock(curproc);
        err = signal_send(curproc, signo);
        proc_unlock(curproc);
    } else if (pid == -1) {
        queue_lock(procQ);
        queue_foreach(proc_t *, proc, procQ) {
            if (proc == curproc)
                continue;

            proc_lock(proc);
            if ((err = signal_send(proc, signo))) {
                proc_unlock(proc);
                queue_unlock(procQ);
                return err;
            }
            proc_unlock(proc);
        }
        queue_unlock(procQ);

        proc_lock(curproc);
        err = signal_send(curproc, signo);
        proc_unlock(curproc);
    } else {
        pgid = -pid;

        queue_lock(procQ);
        queue_foreach(proc_t *, proc, procQ) {
            if (proc == curproc)
                continue;

            proc_lock(proc);
            if (proc->pgid == pgid) {
                if ((err = signal_send(proc, signo))) {
                    proc_unlock(proc);
                    queue_unlock(procQ);
                    return err;
                }
            }
            proc_unlock(proc);
        }
        queue_unlock(procQ);

        proc_lock(curproc);
        err = signal_send(curproc, signo);
        proc_unlock(curproc);
    }

    return err;
}

int pthread_kill(tid_t tid, int signo) {
    int         err     = 0;
    siginfo_t   *info   = NULL;
    thread_t    *thread = NULL;

    if (SIGBAD(signo))
        return -EINVAL;

    if ((err = thread_get(tid, 0, &thread)))
        return err;

    if (signo == 0) {
        thread_unlock(thread);
        return 0;
    }

    if (NULL == (info = (siginfo_t *)kmalloc(sizeof *info))) {
        thread_putref(thread);
        thread_unlock(thread);
        return -ENOMEM;
    }

    memset(info, 0, sizeof *info);

    info->si_addr   = NULL;
    info->si_signo  = signo;
    info->si_pid    = getpid();

    if ((err = thread_sigqueue(thread, info))) {
        thread_putref(thread);
        thread_unlock(thread);
        goto error;
    }

    thread_putref(thread);
    thread_unlock(thread);
    return 0;
error:
    if (info)
        kfree(info);

    return err;
}
