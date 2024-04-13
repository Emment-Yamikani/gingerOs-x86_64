#include <arch/signal.h>
#include <arch/thread.h>
#include <sys/_signal.h>


void arch_signal_return(void) {
#if defined (__x86_64__)
    x86_64_signal_return();
#endif
}

int arch_signal_dispatch(
    arch_thread_t   *thread,
    thread_entry_t  entry,
    siginfo_t       *info,
    sigaction_t     *sigact,
    sigset_t        sigmask
) {
#if defined (__x86_64__)
    return x86_64_signal_dispatch(
        thread,
        entry,
        info,
        sigact,
        sigmask
    );
#endif
}