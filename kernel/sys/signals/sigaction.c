#include <arch/cpu.h>
#include <bits/errno.h>
#include <sys/thread.h>
#include <sys/_signal.h>
#include <mm/kalloc.h>
#include <sys/sysproc.h>
#include <arch/thread.h>
#include <arch/signal.h>

int sigaction(int signo, const sigaction_t *restrict act, sigaction_t *restrict oact) {
    int         err         = -EINVAL;
    sig_desc_t  *sigdesc    = NULL;

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