#pragma once

#include <arch/cpu.h>
#include <arch/x86_64/system.h>
#include <core/assert.h>
#include <sys/system.h>

static inline void pushcli(void) {
    uint64_t intena = is_intena();
    cli();
    if (intena)
        cpu->intena = intena;
    cpu->ncli++;
}

static inline void popcli(void) {
    assert(!is_intena(), "error: interrupts enabled before popcli()!");
    assert((cpu->ncli >= 1), "error: ncli == %d\n", cpu->ncli);
    if ((--cpu->ncli == 0) && cpu->intena) {
        cpu->intena = 0;
        sti();
    }
}