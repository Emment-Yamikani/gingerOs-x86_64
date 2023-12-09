#pragma once

#include <lib/types.h>

int i64_uthread_init(arch_thread_t *arch, thread_entry_t entry, void *arg);
int i64_kthread_init(arch_thread_t *arch, thread_entry_t entry, void *arg);