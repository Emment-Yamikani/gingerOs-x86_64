#pragma once

#include <lib/stddef.h>
#include <arch/x86_64/context.h>
#include <lib/types.h>

typedef struct {
    tf_t        *t_tf;      // thread's arch specific trapframe.
    context_t   *t_ctx0;     // thread's context.
    context_t   *t_ctx1;    // thread's saved context
    tf_t        t_savedtf;  // saved trapframe.
    uintptr_t   t_kstack;   // kernel stack for this thread.
    size_t      t_kstacksz; // size of kernel stack.
    uintptr_t   t_sig_kstack;   // kernel stack for this thread.
    size_t      t_sig_kstacksz; // size of kernel stack.
    thread_t    *t_thread;  // pointer to the thread control block.
    void        *t_priv;    // thread private data.
} x86_64_thread_t;

/**
 * \brief exit execution of thread.
 * \param exit_code exit code from thread.
*/
void arch_thread_exit(uintptr_t exit_code);

/** 
 * \brief Initialize architecture-specific thread data.
 * \param thread arch-specific thread struct. \todo change to (void *)
 * \param entry thread entry point.
 * \param arg argument to be passed to the thread.
 * \return 0 if successful and errno on failure.
*/
int arch_thread_init(x86_64_thread_t *thread, void *(*entry)(void *), void *arg);