#pragma once

#include <arch/x86_64/context.h>


typedef struct {
    tf_t        *t_tf;      // thread's arch specific trapframe.
    context_t   *t_ctx;     // thread's context.
    tf_t        t_savedtf;  // saved trapframe.
    uintptr_t   t_kstack;   // kernel stack for this thread.
    void        *t_priv;    // thread private data.
} x86_64_thread_t;

void arch_thread_stop(void);
void arch_thread_exit(uintptr_t status);
int arch_kthread_init(x86_64_thread_t *thread, void *(*entry)(void *), void *arg);