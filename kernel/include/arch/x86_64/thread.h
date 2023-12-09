#pragma once

#include <lib/types.h>

int x86_64_uthread_init(arch_thread_t *arch, thread_entry_t entry, void *arg);
int x86_64_kthread_init(arch_thread_t *arch, thread_entry_t entry, void *arg);
int x86_64_thread_execve(arch_thread_t *arch, thread_entry_t entry,
                       int argc, const char *argp[], const char *envp[]);