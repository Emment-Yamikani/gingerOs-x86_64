#include <arch/signal.h>
#include <arch/thread.h>
#include <arch/traps.h>
#include <lib/stddef.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <sys/sysproc.h>
#include <sys/_signal.h>
#include <sys/thread.h>
#include <sys/proc.h>

const char *signal_str[] = {
    [SIGABRT - 1]   = "SIGABRT",
    [SIGALRM - 1]   = "SIGALRM",
    [SIGBUS - 1]    = "SIGBUS",
    [SIGCANCEL - 1] = "SIGCANCEL",
    [SIGCHLD - 1]   = "SIGCHLD",
    [SIGCONT - 1]   = "SIGCONT",
    [SIGEMT - 1]    = "SIGEMT",
    [SIGFPE - 1]    = "SIGFPE",
    [SIGHUP - 1]    = "SIGHUP",
    [SIGILL - 1]    = "SIGILL",
    [SIGINT - 1]    = "SIGINT",
    [SIGIO - 1]     = "SIGIO",
    [SIGIOT - 1]    = "SIGIOT",
    [SIGKILL - 1]   = "SIGKILL",
    [SIGPIPE - 1]   = "SIGPIPE",
    [SIGPROF - 1]   = "SIGPROF",
    [SIGQUIT - 1]   = "SIGQUIT",
    [SIGSEGV - 1]   = "SIGSEGV",
    [SIGSTOP - 1]   = "SIGSTOP",
    [SIGSYS - 1]    = "SIGSYS",
    [SIGTERM - 1]   = "SIGTERM",
    [SIGTRAP - 1]   = "SIGTRAP",
    [SIGTSTP - 1]   = "SIGTSTP",
    [SIGTTIN - 1]   = "SIGTTIN",
    [SIGTTOU - 1]   = "SIGTTOU",
    [SIGURG - 1]    = "SIGURG",
    [SIGUSR1 - 1]   = "SIGUSR1",
    [SIGUSR2 - 1]   = "SIGUSR2",
    [SIGVTALRM - 1] = "SIGVTALRM",
    [SIGWINCH - 1]  = "SIGWINCH",
    [SIGXCPU - 1]   = "SIGXCPU",
    [SIGXFSZ - 1]   = "SIGXFSZ",
};

static int sig_defaults[] = {
    [SIGABRT - 1]   = SIG_TERM_CORE,    // | terminate+core
    [SIGALRM - 1]   = SIG_TERM,         // | terminate
    [SIGBUS - 1]    = SIG_TERM_CORE,    // | terminate+core
    [SIGCANCEL - 1] = SIG_IGNORE,       // | ignore
    [SIGCHLD - 1]   = SIG_IGNORE,       // | ignore
    [SIGCONT - 1]   = SIG_CONT,         // | continue/ignore
    [SIGEMT - 1]    = SIG_TERM_CORE,    // | terminate+core
    [SIGFPE - 1]    = SIG_TERM_CORE,    // | terminate+core
    [SIGHUP - 1]    = SIG_TERM,         // | terminate
    [SIGILL - 1]    = SIG_TERM_CORE,    // | terminate+core
    [SIGINT - 1]    = SIG_TERM,         // | terminate
    [SIGIO - 1]     = SIG_TERM,         // | terminate/ignore
    [SIGIOT - 1]    = SIG_TERM_CORE,    // | terminate+core
    [SIGKILL - 1]   = SIG_TERM,         // | terminate
    [SIGPIPE - 1]   = SIG_TERM,         // | terminate
    [SIGPROF - 1]   = SIG_TERM,         // | terminate
    [SIGQUIT - 1]   = SIG_TERM_CORE,    // | terminate+core
    [SIGSEGV - 1]   = SIG_TERM_CORE,    // | terminate+core
    [SIGSTOP - 1]   = SIG_STOP,         // | stop process
    [SIGSYS - 1]    = SIG_TERM_CORE,    // | terminate+core
    [SIGTERM - 1]   = SIG_TERM,         // | terminate
    [SIGTRAP - 1]   = SIG_TERM_CORE,    // | terminate+core
    [SIGTSTP - 1]   = SIG_STOP,         // | stop process
    [SIGTTIN - 1]   = SIG_STOP,         // | stop process
    [SIGTTOU - 1]   = SIG_STOP,         // | stop process
    [SIGURG - 1]    = SIG_IGNORE,       // | ignore
    [SIGUSR1 - 1]   = SIG_TERM,         // | terminate
    [SIGUSR2 - 1]   = SIG_TERM,         // | terminate
    [SIGVTALRM - 1] = SIG_TERM,         // | terminate
    [SIGWINCH - 1]  = SIG_IGNORE,       // | ignore
    [SIGXCPU - 1]   = SIG_TERM,         // | teminate or terminate+core
    [SIGXFSZ - 1]   = SIG_TERM,         // | teminate or terminate+core
};

int sig_desc_alloc(sig_desc_t **pdesc) {
    sig_desc_t  *sigdesc = NULL;

    if (pdesc == NULL)
        return -EINVAL;
    
    if (NULL == (sigdesc = (sig_desc_t *)kmalloc(sizeof (sig_desc_t))))
        return -ENOMEM;
    
    memset(sigdesc, 0, sizeof *sigdesc);

    sigemptyset(&sigdesc->sig_mask);
    sigdesc->sig_lock = SPINLOCK_INIT();

    for (size_t i = 0; i < NELEM(sigdesc->sig_queue); ++i) {
        sigdesc->sig_queue[i] = QUEUE_INIT();
        sigdesc->sig_action[i].sa_handler = SIG_DFL;
    }

    *pdesc = sigdesc;
    return 0;
}

void sig_desc_free(sig_desc_t *sigdesc) {
    assert(sigdesc, "No signal description\n");

    if (!spin_islocked(&sigdesc->sig_lock))
        spin_lock(&sigdesc->sig_lock);
    
    sigemptyset(&sigdesc->sig_mask);
    
    for (size_t i = 0; i < NELEM(sigdesc->sig_queue); ++i) {
        queue_lock(&sigdesc->sig_queue[i]);
        // instead of flushing, dequeue and free siginfo_t *.
        queue_flush(&sigdesc->sig_queue[i]);
        queue_unlock(&sigdesc->sig_queue[i]);
    }

    spin_unlock(&sigdesc->sig_lock);
    kfree(sigdesc);
}

void sigdequeue_pending(queue_t *queue, siginfo_t **ret) {
    assert(ret, "No return referecnt poiner for signal info\n");

    queue_assert_locked(queue);
    dequeue(queue, (void **)ret);
}

int sigenqueue_pending(queue_t *sigqueue, siginfo_t *info) {
    if (info == NULL || SIGBAD(info->si_signo))
        return -EINVAL;
    queue_assert_locked(sigqueue);
    return enqueue(sigqueue, (void *)info, 1, NULL);
}

int pause(void) {
    return -ENOSYS;
}

int signal_perm(proc_t *proc, uid_t *ppuid) {
    uid_t uid = 0;

    proc_assert_locked(proc);
    proc_assert_locked(curproc);

    if (proc != curproc) {
        cred_lock(curproc->cred);
        if ((curproc->pid == 0) ||
            ((uid = curproc->cred->c_uid) == 0) ||
            (curproc->cred->c_euid == 0) ||
            (curproc->cred->c_gid == 0) ||
            (curproc->cred->c_egid == 0)) {
            cred_unlock(curproc->cred);
            if (ppuid)
                *ppuid = uid;
            return 0;
        }

        cred_lock(proc->cred);
        if ((proc->cred->c_uid != (uid = curproc->cred->c_uid)) &&
            (proc->cred->c_euid != curproc->cred->c_euid) &&
            (proc->cred->c_gid != curproc->cred->c_gid) &&
            (proc->cred->c_egid != curproc->cred->c_egid)) {
            cred_unlock(proc->cred);
            cred_unlock(curproc->cred);
            return -EPERM;
        }

        cred_unlock(proc->cred);
        cred_unlock(curproc->cred);
    }

    if (ppuid)
        *ppuid = uid;
    return 0;
}

int signal_send(proc_t *proc, int signo) {
    int         err         = 0;
    uid_t       uid         = 0;
    siginfo_t   *info       = NULL;
    sig_desc_t  *sigdesc    = NULL;

    if (signo < 0 || signo > NSIG)
        return -EINVAL;

    proc_assert_locked(proc);
    proc_assert_locked(curproc);

    if ((err = signal_perm(proc, &uid)))
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
    info->si_pid    = curproc->pid;
    info->si_value  = (union sigval) {
        .sigval_int = signo
    };

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
    return 0;
}

int kill(pid_t pid, int signo) {
    int     err     = 0;
    proc_t  *proc   = NULL;


    if (pid > 0) { // send to process whose process ID == pid.
        if (pid == getpid()) { // self signal was implied.
            proc_lock(curproc);
            proc = curproc; // get a reference to self
        } else if ((err = procQ_search_bypid(pid, &proc)))
            return err;

        // Not sending to self so lock current process struct.
        if (proc != curproc)
            proc_lock(curproc);
        
        // send the signal to the process in question
        if ((err = signal_send(proc, signo))) {
            // FIXME: consider divating to an error: lable.
            if (proc != curproc)
                proc_unlock(curproc);
            proc_unlock(proc);
            return err;
        }

        if (proc != curproc)
            proc_unlock(curproc);
        
        proc_unlock(proc);

    } else if (pid == 0) { // send to all processes whose pgid == that of sending process' pgid
        proc_lock(curproc);
        // foreach proc, send a signal if it's pgid match sending proc's pgid
        queue_foreach(proc_t *, proc, procQ) {
            // skip the current proc
            if (proc == curproc)
                continue;
            
            proc_lock(proc);
            if (proc->pgid == curproc->pgid) {
                if ((err = signal_send(proc, signo))) {
                    // FIXME: consider divating to an error: lable.
                    proc_unlock(proc);
                    proc_unlock(curproc);
                    return err;
                }
            }
            proc_unlock(proc);
        }
        proc_unlock(curproc);
    } else if (pid == -1) { // send to all process, except an unspecified set of sysprocs.
        proc_lock(curproc);
        queue_foreach(proc_t *, proc, procQ) {
            if (proc == curproc) // skip current proc.
                continue;
            
            proc_lock(proc);
            if ((err = signal_send(proc, signo))) {
                // FIXME: consider divating to an error: lable.
                proc_unlock(proc);
                proc_unlock(curproc);
                return err;
            }
            proc_unlock(proc);
        }
        proc_unlock(curproc);
    } else { // PID < -1, send to all processes whose PGID == ABS(pid).
        proc_lock(curproc);
        // foreach proc, send a signal if it's pgid match sending proc's pgid
        queue_foreach(proc_t *, proc, procQ) {
            // skip the current proc
            if (proc == curproc)
                continue;
            
            proc_lock(proc);
            if (proc->pgid == ABSi(pid)) {
                if ((err = signal_send(proc, signo))) {
                    // FIXME: consider divating to an error: lable.
                    proc_unlock(proc);
                    proc_unlock(curproc);
                    return err;
                }
            }
            proc_unlock(proc);
        }
        proc_unlock(curproc);
    }

    return 0;
}

unsigned long alarm(unsigned long sec) {
    (void)sec;

    return -ENOSYS;
}

sigfunc_t signal(int signo, sigfunc_t func) {
    sigaction_t act, oact;
    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (signo == SIGALRM) {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif
    }
    else
    {
        act.sa_flags |= SA_RESTART;
    }
    if (sigaction(signo, &act, &oact) < 0)
        return (SIG_ERR);
    return (oact.sa_handler);
}

int sigpending(sigset_t *sig_mask) {
    sig_desc_t *sigdesc = NULL;

    if (sig_mask == NULL)
        return -EINVAL;

    sigemptyset(sig_mask);
    sigdesc = current->t_sigdesc;

    sigdesc_lock(sigdesc);
    for (int signo = 0; signo < NSIG; ++signo) {
        queue_lock(&sigdesc->sig_queue[signo]);
        if (queue_count(&sigdesc->sig_queue[signo]))
            sigaddset(sig_mask, (signo + 1));
        queue_unlock(&sigdesc->sig_queue[signo]);
    }
    sigdesc_unlock(sigdesc);

    return 0;
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

int sigwait(const sigset_t *restrict sig_mask, int *restrict signop) {
    sigset_t    sset, oset;
    int         err     = 0;
    int         signo   = 1;
    siginfo_t   *info   = NULL;

    printk("FIXME %s:%d: in %s()\n", __FILE__, __LINE__, __func__);

    if (sig_mask == NULL)
        return -EINVAL;

    sset = *sig_mask;

    if ((err = pthread_sigmask(SIG_UNBLOCK, &sset, &oset)))
        return err;

    loop() {
        current_lock();
        if (sigismember(&sset, signo) == 1) {
            queue_lock(&current->t_sigqueue[signo - 1]);
            sigdequeue_pending(&current->t_sigqueue[signo - 1], &info);
            queue_unlock(&current->t_sigqueue[signo - 1]);

            if (info != NULL) {
                current_unlock();
                kfree(info);
                break;
            }
        }
        current_unlock();

        if (signo++ > NSIG)
            signo = 1;
    }

    pthread_sigmask(SIG_SETMASK, &oset, NULL);

    if (signop)
        *signop = signo;
    return 0;
}

int pthread_sigmask(int how, const sigset_t *restrict sig_mask, sigset_t *restrict oset) {
    int err = 0;
    current_lock();
    err = thread_sigmask(current, how, sig_mask, oset);
    current_unlock();
    return err;
}

int sigprocmask(int how, const sigset_t *restrict sig_mask, sigset_t *restrict oset) {
    int         err     = 0;
    sig_desc_t  *sigdesc   = NULL;

    current_lock();
    sigdesc = current->t_sigdesc;
    current_unlock();

    sigdesc_lock(sigdesc);

    if (oset)
        *oset = sigdesc->sig_mask;

    if (sig_mask == NULL) {
        sigdesc_unlock(sigdesc);
        return 0;
    }

    if (sigismember(sig_mask, SIGKILL) || sigismember(sig_mask, SIGSTOP)) {
        sigdesc_unlock(sigdesc);
        return -EINVAL;
    }

    switch (how) {
    case SIG_BLOCK:
        sigdesc->sig_mask |= *sig_mask;
        break;
    case SIG_UNBLOCK:
        sigdesc->sig_mask &= ~*sig_mask;
        break;
    case SIG_SETMASK:
        sigdesc->sig_mask = *sig_mask;
        break;
    default:
        err = -EINVAL;
        break;
    }

    sigdesc_unlock(sigdesc);
    return err;
}

int sigaction(int signo, const sigaction_t *restrict act, sigaction_t *restrict oact) {
    sig_desc_t  *sigdesc    = NULL;
    int         err         = -EINVAL;

    if (SIGBAD(signo))
        return -EINVAL;

    current_lock();
    sigdesc = current->t_sigdesc;
    current_unlock();

    sigdesc_lock(sigdesc);

    if (oact)
        *oact = sigdesc->sig_action[signo - 1];

    if (act == NULL) {
        sigdesc_unlock(sigdesc);
        return 0;
    }

    if ((signo == SIGSTOP) || (signo == SIGKILL))
            goto error;

    if (!act->sa_handler && !act->sa_sigaction)
        goto error;

    switch ((uintptr_t)act->sa_handler) {
    case SIG_IGNORE:
        break;
    case SIG_ABRT:
        __fallthrough;
    case SIG_TERM:
        __fallthrough;
    case SIG_TERM_CORE:
        __fallthrough;
    case SIG_STOP:
        __fallthrough;
    case SIG_CONT:
        err = -EINVAL;
        goto error;
    }

    switch ((uintptr_t)act->sa_sigaction) {
    case SIG_IGNORE:
        break;
    case SIG_ABRT:
        __fallthrough;
    case SIG_TERM:
        __fallthrough;
    case SIG_TERM_CORE:
        __fallthrough;
    case SIG_STOP:
        __fallthrough;
    case SIG_CONT:
        err = -EINVAL;
        goto error;
    }

    if ((act->sa_flags & SA_SIGINFO) && (act->sa_sigaction == NULL))
        goto error;

    sigdesc->sig_action[signo - 1] = *act;
    sigdesc_unlock(sigdesc);
    return 0;
error:
    sigdesc_unlock(sigdesc);
    return err;
}

int signal_dispatch(void) {
    int             err         = 0;
    sigset_t        oset        = 0;
    flags32_t       flags       = 0;
    sigaction_t     act         = {0};
    sig_desc_t      *desc       = NULL;
    siginfo_t       *info       = NULL;
    arch_thread_t   *tarch      = NULL;
    sigfunc_t       handler     = NULL;

    // prepare signal masks/
    sigemptyset(&oset);

    current_lock();
    oset = current->t_sigmask;
    desc = current->t_sigdesc;
    thread_sigdequeue(current, &info);
    current_unlock();

    sigdesc_lock(desc);
    if (info) {
        flags = 1;
        act = desc->sig_action[info->si_signo - 1];
        sigdesc_unlock(desc);
        goto __handle_signal;
    }

    for (int signo = 0; signo < NSIG; ++signo) {
        current_lock();
        err = sigismember(&current->t_sigmask, signo + 1);
        current_unlock();

        // signal is masked internally in the thread.
        if (err == 1)
            continue;

        queue_lock(&desc->sig_queue[signo]);
        dequeue(&desc->sig_queue[signo], (void **)&info);
        queue_unlock(&desc->sig_queue[signo]);
        if (info) {
            // save this signal mask.
            oset    = desc->sig_mask;
            // get the sigaction info for this signal.
            act     = desc->sig_action[signo];
            // block recursive signals of this type and those already in sig_mask.
            sigaddset(&desc->sig_mask, info->si_signo - 1);
            break;
        }
    }
    sigdesc_unlock(desc);

    if (info == NULL) { // no signal pending...
        return 0;
    }

__handle_signal:
    handler = (act.sa_flags & SA_SIGINFO ? (sigfunc_t)act.sa_sigaction : act.sa_handler);
    
    switch ((uintptr_t)handler) {
    case (uintptr_t)SIG_DFL:
        switch (sig_defaults[info->si_signo - 1]) {
        case SIG_TERM:
            goto __terminate;
        case SIG_ABRT:
        case SIG_CONT:
        case SIG_STOP:
        case SIG_TERM_CORE:
        assert_msg(0,
            "%s:%d: SIG_DFL is default action for signo(%d)",
            __FILE__,
            __LINE__,
            info->si_signo
        );
        case SIG_IGNORE:
            goto __ignore;
        break;
        }
        break;
    case (uintptr_t)SIG_ERR:
        assert_msg(0,
            "%s:%d: SIG_ERR is default action for signo(%d)",
            __FILE__,
            __LINE__,
            info->si_signo
        );
        break;
    case (uintptr_t)SIG_IGN:
__ignore:
        assert_msg(0,
            "%s:%d: SIG_IGN is default action for signo(%d)",
            __FILE__,
            __LINE__,
            info->si_signo
        );
        goto __exit_handler;
    }

    current_lock();

    tarch = &current->t_arch;
    tarch->t_uctx->uc_sigmask = oset;

    // dump_tf(&tarch->t_uctx->uc_mcontext, 0);
    // debugloc();
    assert_msg(0 == (err =
        arch_signal_dispatch(tarch, (void *)handler, info, &act)),
        "%s:%d: Failed to dispatch_signal, err: %d\n", __FILE__, __LINE__, err
    );
    // dump_tf(&tarch->t_uctx->uc_mcontext, 0);
    // debugloc();

    current_unlock();
    // FIXME: consider ref counted info struct.
    kfree(info); // free siginfo_t *info (struct).

__exit_handler:
    if (flags & 1) // restore per-thread sig_mask.
        pthread_sigmask(SIG_SETMASK, &oset, NULL);
    else // restore global sig_mask.
        sigprocmask(SIG_SETMASK, &oset, NULL);
    return 0;
__terminate:
    kfree(info); // FIXME: use a refcnt-based info struct.
    if (flags & 1) // restore per-thread sig_mask.
        pthread_sigmask(SIG_SETMASK, &oset, NULL);
    else // restore global sig_mask.
        sigprocmask(SIG_SETMASK, &oset, NULL);
    exit(EINTR);
    return 0;
}