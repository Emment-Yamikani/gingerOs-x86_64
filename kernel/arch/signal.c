#include <sys/_signal.h>
#include <arch/signal.h>

void arch_signal_return(void) {
#if defined (__x86_64__)
    x86_64_signal_return();
#endif
}