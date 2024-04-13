#pragma once

#include <arch/x86_64/thread.h>
#include <lib/stdint.h>
#include <lib/stddef.h>
#include <arch/ucontext.h>
#include <sys/_signal.h>

/**
 * Arch-specific thread structure.
 * This is what the architecture understands a thread to be.
 * i.e, thread_t is understood by the multitasking concept in ginger
 * but arch_thread_t is what it really is underneath.
 *
 * NOTE: t_contextand t_ucontext;
 * t_context: is what is used by the system to switch contexts
 * by calling swtch() implicitly through sched() or in schedule().
 * see sys/sched/sched.c
 * t_ucontext: is the execution context at the time of
 * interrupts, exceptions, and system calls.
 */
typedef struct __arch_thread_t {
    thread_t    *t_thread;      // pointer to thread control block.
    context_t   *t_context;     // caller-callee context.
    ucontext_t  *t_ucontext;    // execution context status
    flags64_t   t_flags;        // flags.
    uc_stack_t  t_sstack;       // scratch stack for when executing for the first time.
    uc_stack_t  t_kstack;       // kernel stack description.
    uc_stack_t  t_ustack;       // user stack description.
    uc_stack_t  t_sigaltstack;  // if SA_ONSTACK is set for a signal handler, use this stack.
} arch_thread_t;

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
int arch_kthread_init(arch_thread_t *thread, thread_entry_t entry, void *arg);
int arch_uthread_init(arch_thread_t *thread, thread_entry_t entry, void *arg);
int x86_64_signal_dispatch( arch_thread_t   *thread, thread_entry_t  entry,
    siginfo_t *info, sigaction_t *sigact, sigset_t sigmask
);

void arch_exec_free_copy(char ***arg_env);
char ***arch_execve_copy(char *_argp[], char *_envp[]);
int arch_thread_execve(arch_thread_t *thread, thread_entry_t entry,
    int argc, const char *argp[], const char *envp[]
);

int arch_thread_setkstack(arch_thread_t *arch);
int arch_thread_fork(arch_thread_t *dst, arch_thread_t *src);