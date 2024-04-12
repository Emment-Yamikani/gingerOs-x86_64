#include <arch/signal.h>
#include <arch/thread.h>
#include <sys/_signal.h>


void arch_signal_return(void) {
#if defined (__x86_64__)
    x86_64_signal_return();
#endif
}