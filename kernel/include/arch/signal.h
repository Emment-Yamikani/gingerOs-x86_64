#pragma once

#include <arch/x86_64/signal.h>
#include <lib/types.h>
#include <sys/_signal.h>
#include <arch/thread.h>

int arch_signal_dispatch(
    arch_thread_t   *tarch,
    thread_entry_t  entry,
    siginfo_t       *info,
    sigaction_t     *sigact
);

void arch_signal_return(void);