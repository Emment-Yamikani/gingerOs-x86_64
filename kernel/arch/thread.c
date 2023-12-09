#include <arch/thread.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <sys/thread.h>
#include <sys/_signal.h>

int arch_uthread_init(arch_thread_t *arch, thread_entry_t entry, void *arg) {
#if defined __x86_64__
    return i64_uthread_init(arch, entry, arg);
#endif
}

int arch_kthread_init(arch_thread_t *arch, thread_entry_t entry, void *arg) {
#if defined __x86_64__
    return i64_kthread_init(arch, entry, arg);
#endif
}