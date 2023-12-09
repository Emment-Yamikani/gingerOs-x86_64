#pragma once

#include <arch/x86_64/context.h>
#include <arch/x86_64/thread.h>
#include <lib/stdint.h>
#include <lib/stddef.h>

typedef struct arch_thread_t {
#if defined __x86_64__
    tf_t        *t_tf;          // thread's arch specific trapframe.
    context_t   *t_ctx0;        // thread's context.
    context_t   *t_ctx1;        // thread's saved context
    tf_t        t_savedtf;      // saved trapframe.
#endif
    void        *t_priv;        // thread private data.
    uintptr_t   t_kstack;       // kernel stack for this thread.
    size_t      t_kstacksz;     // size of kernel stack.
    uintptr_t   t_sig_kstack;   // kernel stack for this thread.
    size_t      t_sig_kstacksz; // size of kernel stack.
    thread_t    *t_thread;      // pointer to the thread control block.
    vmr_t       *t_ustack;      // thread user stack.
    vmr_t       *t_altsigstack; // thread alternate signal stack.
} arch_thread_t;

int arch_uthread_init(arch_thread_t *arch_thread, thread_entry_t entry, void *arg);

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
int arch_kthread_init(arch_thread_t *arch_thread, thread_entry_t entry, void *arg);

void arch_exec_free_copy(char ***arg_env);
char ***arch_execve_copy(char *_argp[], char *_envp[]);

int arch_thread_execve(arch_thread_t *thread, thread_entry_t entry, int argc, const char *argp[], const char *envp[]);