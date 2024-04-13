#include <arch/signal.h>
#include <arch/thread.h>
#include <arch/traps.h>
#include <lib/stddef.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <sys/sysproc.h>
#include <sys/_signal.h>
#include <sys/thread.h>

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

__unused
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

    for (size_t i = 0; i < NELEM(sigdesc->sig_queue); ++i)
        sigdesc->sig_queue[i] = QUEUE_INIT();
    
    for (size_t i = 0; i < NELEM(sigdesc->sig_action); ++i)
        sigdesc->sig_action[i].sa_handler = SIG_DFL;

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
    int err = 0;

    if (info == NULL || SIGBAD(info->si_signo))
        return -EINVAL;

    queue_assert_locked(sigqueue);
    err = enqueue(sigqueue, (void *)info, 1, NULL);
    return err;
}

int pause(void) {
    return -ENOSYS;
}

int kill(pid_t pid, int signo) {
    (void)pid, (void)signo;

    return -ENOSYS;
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

    current_tgroup_lock();
    if ((err = tgroup_get_thread(current_tgroup(), tid, 0, &thread))) {
        current_tgroup_unlock();
        return err;
    }
    current_tgroup_unlock();

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
    int         err     = -EINVAL;
    sig_desc_t  *sigdesc   = NULL;

    if (SIGBAD(signo))
        return -EINVAL;

    current_lock();
    sigdesc = current->t_sigdesc;
    current_unlock();

    if (oact)
        *oact = sigdesc->sig_action[signo - 1];

    if (act == NULL) {
        sigdesc_unlock(sigdesc);
        return 0;
    }

    if (((signo == SIGSTOP) ||
        (signo == SIGKILL)) &&
        (act->sa_handler || act->sa_handler))
            goto error;

    if (!act->sa_handler && !act->sa_handler)
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

int dispatch_signal(void) {
    int             err         = 0;
    flags32_t       flags       = 0;
    sigset_t        set         = 0;
    sigset_t        oset        = 0;
    sigset_t        tset        = 0;
    sigaction_t     act         = {0};
    sig_desc_t      *desc       = NULL;
    siginfo_t       *info       = NULL;
    sigfunc_t       handler     = NULL;
    arch_thread_t   *tarch      = NULL;

    // prepare signal masks/
    sigemptyset(&set);
    sigemptyset(&oset);
    sigemptyset(&tset);

    current_lock();
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
    if (flags & 1) { // is this an internal signal.
        sigemptyset(&set);
        sigaddset(&set, info->si_signo - 1);
        pthread_sigmask(SIG_BLOCK, &set, &tset);
    }

    handler = (act.sa_flags & SA_SIGINFO ? (sigfunc_t)act.sa_sigaction : act.sa_handler);
    if (handler == SIG_DFL) { // perform defualt action?
        switch (sig_defaults[info->si_signo - 1]) {
        case SIG_ABRT:
        case SIG_CONT:
        case SIG_STOP:
        case SIG_TERM:
        case SIG_IGNORE:
        case SIG_TERM_CORE:
        break;
        }
    } else if (handler == SIG_IGN) { // ignore this signal?
        goto __exit_handler;
    } else
        panic("How did we write SIG_ERR in handler?\n");
    
    tarch = &current->t_arch;
    err = arch_signal_dispatch(
        tarch,
        (void *)handler,
        info,
        &act, (flags & 1) ? tset : oset
    );

    assert(err == 0, "Failed to initialize signal handler");

    kfree(info); // free siginfo_t *info (struct).

__exit_handler:
    if (flags & 1) // restore per-thread sig_mask.
        pthread_sigmask(SIG_SETMASK, &tset, NULL);
    else // restore global sig_mask.
        sigprocmask(SIG_SETMASK, &oset, NULL);
    return 0;
}