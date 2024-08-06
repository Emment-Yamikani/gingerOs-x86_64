#include <arch/cpu.h>
#include <bits/errno.h>
#include <sys/thread.h>
#include <sys/_signal.h>
#include <mm/kalloc.h>
#include <sys/sysproc.h>
#include <arch/thread.h>
#include <arch/signal.h>

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