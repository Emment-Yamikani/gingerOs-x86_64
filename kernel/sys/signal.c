#include <sys/_signal.h>
#include <lib/stddef.h>
#include <sys/thread.h>
#include <arch/x86_64/context.h>
#include <arch/traps.h>

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

static int sig_defaults[] = {
    [SIGABRT] = SIG_TERM_CORE,   // | terminate+core
    [SIGALRM] = SIG_TERM,        // | terminate
    [SIGBUS] = SIG_TERM_CORE,    // | terminate+core
    [SIGCANCEL] = SIG_IGNORE,    // | ignore
    [SIGCHLD] = SIG_IGNORE,      // | ignore
    [SIGCONT] = SIG_CONT,        // | continue/ignore
    [SIGEMT] = SIG_TERM_CORE,    // | terminate+core
    [SIGFPE] = SIG_TERM_CORE,    // | terminate+core
    [SIGHUP] = SIG_TERM,         // | terminate
    [SIGILL] = SIG_TERM_CORE,    // | terminate+core
    [SIGINT] = SIG_TERM,         // | terminate
    [SIGIO] = SIG_TERM,          // | terminate/ignore
    [SIGIOT] = SIG_TERM_CORE,    // | terminate+core
    [SIGKILL] = SIG_TERM,        // | terminate
    [SIGPIPE] = SIG_TERM,        // | terminate
    [SIGPROF] = SIG_TERM,        // | terminate
    [SIGQUIT] = SIG_TERM_CORE,   // | terminate+core
    [SIGSEGV] = SIG_TERM_CORE,   // | terminate+core
    [SIGSTOP] = SIG_STOP,        // | stop process
    [SIGSYS] = SIG_TERM_CORE,    // | terminate+core
    [SIGTERM] = SIG_TERM,        // | terminate
    [SIGTRAP] = SIG_TERM_CORE,   // | terminate+core
    [SIGTSTP] = SIG_STOP,        // | stop process
    [SIGTTIN] = SIG_STOP,        // | stop process
    [SIGTTOU] = SIG_STOP,        // | stop process
    [SIGURG] = SIG_IGNORE,       // | ignore
    [SIGUSR1] = SIG_TERM,        // | terminate
    [SIGUSR2] = SIG_TERM,        // | terminate
    [SIGVTALRM] = SIG_TERM,      // | terminate
    [SIGWINCH] = SIG_IGNORE,     // | ignore
    [SIGXCPU] = SIG_TERM,        // | teminate or terminate+core
    [SIGXFSZ] = SIG_TERM,        // | teminate or terminate+core
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

    printk("FIXME %s:%D: in %S()\n", __FILE__, __LINE__, __func__);

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

    if (!act->sa_handler && !act->sa_handler)
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

void signal_return(void) {
    current_lock();
    swtch(&current->t_arch.t_ctx0, current->t_arch.t_ctx1);
}

void signal_dispatch(void) {
    current_unlock();
}

int signal_handle(tf_t *tf __unused) {
    void *arg = NULL;
    tf_t *sig_tf = NULL;
    sigaction_t act = {0};
    int err = 0, signo = 0;
    sigfunc_t handler = NULL;
    uintptr_t default_act = 0;
    context_t *sig_ctx = NULL;
    siginfo_t *sig_info = NULL, info;
    uintptr_t *sig_stack = NULL, *stack = NULL;

    current_tgroup_lock();
    current_lock();
    sig_lock(&current_tgroup()->tg_signals);

    for (signo = 0; signo < NSIG; ++signo) {
        if (current_tgroup()->tg_signals.sig_queues[signo]) {
            current_tgroup()->tg_signals.sig_queues[signo]--;
            if ((err = thread_sigqueue(current, signo + 1))) {
                sig_unlock(&current_tgroup()->tg_signals);
                current_unlock();
                current_tgroup_unlock();
                return err;
            }
        }
    }

    sig_unlock(&current_tgroup()->tg_signals);
    current_unlock();
    current_tgroup_unlock();

    current_lock();

    if ((err = signo = thread_sigdequeue(current)) <= 0) {
        current_unlock();
        return err;
    }

    signo -= 1;
    arg = (void *)(uintptr_t)signo + 1;
    act = current_tgroup()->tg_signals.sig_action[signo];
    handler = (act.sa_flags & SA_SIGINFO ? (sigfunc_t)act.sa_sigaction : (sigfunc_t)act.sa_handler);

    if (handler == SIG_DFL)
        default_act = sig_defaults[signo];

    switch (default_act) {
    case SIG_IGNORE:
        assert_msg(0, "%s default action: IGNORE", signal_str[signo]);
        break;
    case SIG_ABRT:
        assert_msg(0, "%s default action: ABORT", signal_str[signo]);
        break;
    case SIG_TERM:
        assert_msg(0, "%s default action: TERMINATE", signal_str[signo]);
        break;
    case SIG_TERM_CORE:
        assert_msg(0, "%s default action: TERMINATE+CORE", signal_str[signo]);
        break;
    case SIG_STOP:
        assert_msg(0, "%s default action: STOP", signal_str[signo]);
        break;
    case SIG_CONT:
        assert_msg(0, "%s default action: CONTINUE", signal_str[signo]);
        break;
    }

    if (!current_isuser()) {
        if (NULL == (sig_stack = stack = (uintptr_t *)thread_alloc_kstack(STACKSZMIN))) {
            current_unlock();
            return -ENOMEM;
        }

        sig_stack = (uintptr_t *)ALIGN16(((uintptr_t)sig_stack + STACKSZMIN));

        if (act.sa_flags & SA_SIGINFO) {
            sig_info = (siginfo_t *)((uintptr_t)sig_stack - sizeof *sig_info);
            arg = sig_info;
            sig_stack = (uintptr_t *)sig_info;
            *sig_info = info;
        }

        *--sig_stack = (uintptr_t)signal_return;
        sig_tf = (tf_t *) ((uintptr_t)sig_stack - sizeof *sig_tf);

        sig_tf->ss = SEG_KDATA64 << 3;
        sig_tf->rsp = (uintptr_t)sig_stack;
        sig_tf->rbp = sig_tf->rsp;
        sig_tf->rflags = LF_IF;
        sig_tf->cs = SEG_KCODE64 << 3;
        sig_tf->rip = (uintptr_t)handler;
        sig_tf->rdi = (uintptr_t)arg;
        sig_tf->fs = SEG_KDATA64 << 3;
        
        sig_stack = (uintptr_t *)sig_tf;
        *--sig_stack = (uintptr_t)trapret;
        sig_ctx = (context_t *)((uintptr_t)sig_stack - sizeof *sig_ctx);
        sig_ctx->rip = (uintptr_t)signal_dispatch;
        sig_ctx->rbp = sig_tf->rsp;

        current->t_arch.t_ctx0 = sig_ctx;
    }

    current_setflags(THREAD_HANDLING_SIG);

    swtch(&current->t_arch.t_ctx1, current->t_arch.t_ctx0);

    if (!current_isuser())
        thread_free_kstack((uintptr_t)stack, STACKSZMIN);

    current_maskflags(THREAD_HANDLING_SIG);

    current_unlock();

    return 0;
}