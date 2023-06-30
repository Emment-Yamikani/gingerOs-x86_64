#pragma once

#include <arch/x86_64/system.h>
#include <sys/system.h>
#include <sync/preempt.h>
#include <sys/system.h>
#include <arch/cpu.h>
#include <lib/printk.h>

#define pushcli() ({               \
    uint64_t intena = is_intena(); \
    cli();                         \
    if (intena)                    \
        cpu->intena = intena;      \
    cpu->ncli++;                   \
})

#define popcli() ({                                                                                \
    assert_msg(!is_intena(), "%s:%d: error: interrupts enabled before popcli()!", __FILE__, __LINE__); \
    assert_msg((cpu->ncli >= 1), "%s:%d: error: ncli == %d\n", __FILE__, __LINE__, cpu->ncli);         \
    if ((--cpu->ncli == 0) && cpu->intena)                                                         \
    {                                                                                              \
        cpu->intena = 0;                                                                           \
        sti();                                                                                     \
    }                                                                                              \
})
