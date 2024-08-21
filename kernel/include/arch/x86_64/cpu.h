#pragma once

#include <core/types.h>

typedef struct cpu_t {
    isize       ncli;       // cli push-depth
    u64         intena;     // were interrupts enabled?
    u64         flags;      // runtime flags for cpu

    thread_t    *thread;    // current thread on this core.
} cpu_t;

// get cpu local storage.
extern cpu_t *getcls(void);


#define cpu         ({ getcls(); })

// current thread on this core.
#define current     ({ cpu ? cpu->thread : NULL; })