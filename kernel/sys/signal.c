#include <sys/_signal.h>
#include <lib/stddef.h>
#include <sys/thread.h>

const char *signal_str[] = {
    [SIGABRT - 1] = "SIGABRT",
    [SIGALRM - 1] = "SIGALRM",
    [SIGBUS - 1] = "SIGBUS",
    [SIGCANCEL - 1] = "SIGCANCEL",
    [SIGCHLD - 1] = "SIGCHLD",
    [SIGCONT - 1] = "SIGCONT",
    [SIGEMT - 1] = "SIGEMT",
    [SIGFPE - 1] = "SIGFPE",
    [SIGHUP - 1] = "SIGHUP",
    [SIGILL - 1] = "SIGILL",
    [SIGINT - 1] = "SIGINT",
    [SIGIO - 1] = "SIGIO",
    [SIGIOT - 1] = "SIGIOT",
    [SIGKILL - 1] = "SIGKILL",
    [SIGPIPE - 1] = "SIGPIPE",
    [SIGPROF - 1] = "SIGPROF",
    [SIGQUIT - 1] = "SIGQUIT",
    [SIGSEGV - 1] = "SIGSEGV",
    [SIGSTOP - 1] = "SIGSTOP",
    [SIGSYS - 1] = "SIGSYS",
    [SIGTERM - 1] = "SIGTERM",
    [SIGTRAP - 1] = "SIGTRAP",
    [SIGTSTP - 1] = "SIGTSTP",
    [SIGTTIN - 1] = "SIGTTIN",
    [SIGTTOU - 1] = "SIGTTOU",
    [SIGURG - 1] = "SIGURG",
    [SIGUSR1 - 1] = "SIGUSR1",
    [SIGUSR2 - 1] = "SIGUSR2",
    [SIGVTALRM - 1] = "SIGVTALRM",
    [SIGWINCH - 1] = "SIGWINCH",
    [SIGXCPU - 1] = "SIGXCPU",
    [SIGXFSZ - 1] = "SIGXFSZ",
};

int pause(void);
int kill(pid_t pid, int signo);
unsigned long alarm(unsigned long sec);

sigfunc_t signal(int signo, sigfunc_t func) {
    sigaction_t act, oact;
    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (signo == SIGALRM) {
    #ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
    #endif
    } else {
        act.sa_flags |= SA_RESTART;
    }
    if (sigaction(signo, &act, &oact) < 0)
        return(SIG_ERR);
    return(oact.sa_handler);
}

int sigpending(sigset_t *set) {
    if (set == NULL)
        return -EINVAL;

    sigemptyset(set);

    current_tgroup_lock();
    sig_lock(&current_tgroup()->tg_signals);

    for (int signo = 0; signo < NSIG; ++signo) {
        if (current_tgroup()->tg_signals.sig_queues[signo])
            sigaddset(set, (signo +1));
    }
    
    sig_unlock(&current_tgroup()->tg_signals);
    current_tgroup_unlock();

    return 0;
}

int pthread_kill(tid_t tid, int signo) {
    int err = 0;
    thread_t *thread = NULL;

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

    if ((err = thread_sigqueue(thread, signo))) {
        thread_unlock(thread);
        goto error;
    }

    thread_unlock(thread);
    return 0;
error:
    return err;
}

int sigwait(const sigset_t *restrict set, int *restrict signop) {
    int err = 0;
    int signo = 1;
    sigset_t sset, oset;
    thread_t *thread =  NULL;

    if (set == NULL)
        return -EINVAL;
    
    sset = *set;

    if ((err = pthread_sigmask(SIG_UNBLOCK, &sset, &oset)))
        return err;
    
    loop () {
        current_lock();
        if (sigismemeber(&sset, signo) == 1) {
            if (current->t_sigqueue[signo - 1]) {
                current->t_sigqueue[signo - 1]--;
                current_unlock();
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

int pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset) {
    int err = 0;

    current_lock();
    if (oset)
        *oset = current->t_sigmask;
    
    if (set == NULL) {
        current_unlock();
        return 0;
    }

    switch (how) {
    case SIG_BLOCK:
        current->t_sigmask |= *set;
        break;
    case SIG_UNBLOCK:
        current->t_sigmask &= ~*set;
        break;
    case SIG_SETMASK:
        current->t_sigmask = *set;
        break;
    default:
        err = -EINVAL;
        break;
    }
    current_unlock();

    return err;
}

int sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset) {
    int err = 0;
    current_tgroup_lock();

    sig_lock(&current_tgroup()->tg_signals);

    if (oset)
        *oset = current_tgroup()->tg_signals.sig_mask;

    if (set == NULL) {
        sig_unlock(&current_tgroup()->tg_signals);
        current_tgroup_unlock();
        return 0;
    }

    switch (how) {
    case SIG_BLOCK:
        current_tgroup()->tg_signals.sig_mask |= *set;
        break;
    case SIG_UNBLOCK:
        current_tgroup()->tg_signals.sig_mask &= ~*set;
        break;
    case SIG_SETMASK:
        current_tgroup()->tg_signals.sig_mask = *set;
        break;
    default:
        err = -EINVAL;
        break;
    }

    sig_unlock(&current_tgroup()->tg_signals);
    current_tgroup_unlock();
    return err;
}

int sigaction(int signo, const sigaction_t *restrict act, sigaction_t *restrict oact) {
    int err = -EINVAL;

    if (SIGBAD(signo))
        return -EINVAL;
    
    
    current_tgroup_lock();
    sig_lock(&current_tgroup()->tg_signals);

    if (oact)
        *oact = current_tgroup()->tg_signals.sig_action[signo -1];

    if (act == NULL) {
        sig_unlock(&current_tgroup()->tg_signals);
        current_tgroup_unlock();
        return 0;
    }

    switch (signo) {
    case SIGSTOP:
        __fallthrough;
    case SIGKILL:
        if (act->sa_handler || act->sa_handler)
            goto error;
        break;
    }

    if (act->sa_handler || act->sa_handler)
        goto error;

    if ((act->sa_flags & SA_SIGINFO) && (act->sa_sigaction == NULL))
        goto error;

    current_tgroup()->tg_signals.sig_action[signo - 1] = *act;

    sig_unlock(&current_tgroup()->tg_signals);
    current_tgroup_unlock();
    return 0;
error:
    sig_unlock(&current_tgroup()->tg_signals);
    current_tgroup_unlock();
    return err;
}