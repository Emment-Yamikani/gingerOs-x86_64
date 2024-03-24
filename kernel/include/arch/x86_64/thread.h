#pragma once

#include <lib/types.h>

extern void x86_64_thread_exit(uintptr_t exit_code);
extern int x86_64_uthread_init(arch_thread_t *arch, thread_entry_t entry, void *arg);
extern int x86_64_kthread_init(arch_thread_t *arch, thread_entry_t entry, void *arg);
extern int x86_64_thread_execve(arch_thread_t *arch, thread_entry_t entry,
                       int argc, const char *argp[], const char *envp[]);
extern int x86_64_thread_setkstack(arch_thread_t *arch);
extern int x86_64_thread_fork(arch_thread_t *dst, arch_thread_t *src);