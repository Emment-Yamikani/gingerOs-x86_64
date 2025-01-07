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
    
    /// get a reference to the signal
    /// desc for the current thread group.
    desc = current->t_sigdesc;
    
    /// Check if we have a thread local signal
    /// by attempting to get signal 'info'.
    thread_sigdequeue(current, &info);
    current_unlock();

    sigdesc_lock(desc); // grab a lock on signal description.
    if (info) {
        /// If we have a valid info.
        /// get the signal action for it.
        act     = desc->sig_action[info->si_signo - 1];
        sigdesc_unlock(desc);
        flags   = 1; // we'll need this flag later.
        goto __handle_signal; // handle signal that was sent to this thread.
    }

    /// No signal was sent to the current thread so,
    /// check through thread group signal desc for a global signal.
    for (int signo = 0; signo < NSIG; ++signo) {
        queue_lock(&desc->sig_queue[signo]);
        if (queue_count(&desc->sig_queue[signo])) {
            current_lock();
            /// Chech if signal is masked internally in the thread.
            if ((sigismember(&desc->sig_mask, signo + 1)) == 1) {
                current_unlock();
                queue_unlock(&desc->sig_queue[signo]);
                /// signal is masked internally in the thread ignore it.
                continue;
            }
            current_unlock();

            sigdequeue_pending(&desc->sig_queue[signo], &info);
            // save this signal mask.
            oset    = desc->sig_mask;
            // get the sigaction info for this signal.
            act     = desc->sig_action[signo];
            // block recursive signals of this type and those already in sig_mask.
            sigaddset(&desc->sig_mask, signo + 1);
        }
        queue_unlock(&desc->sig_queue[signo]);
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
        assert(0, "SIG_DFL is default action for signo(%d)", info->si_signo);
        case SIG_IGNORE:
            goto __ignore;
        break;
        }
        break;
    case (uintptr_t)SIG_ERR:
        assert(0, "SIG_ERR is default action for signo(%d)", info->si_signo);
        break;
    case (uintptr_t)SIG_IGN:
__ignore:
        assert(0, "SIG_IGN is default action for signo(%d)", info->si_signo);
        goto __exit_handler;
    }

    current_lock();

    tarch = &current->t_arch;
    tarch->t_uctx->uc_sigmask = oset;

    // dump_tf(&tarch->t_uctx->uc_mcontext, 0);

    assert(0 == (err = arch_signal_dispatch(tarch, (void *)handler, info, &act)),
        "Failed to dispatch_signal, err: %d\n", err
    );
    // dump_tf(&tarch->t_uctx->uc_mcontext, 0);

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