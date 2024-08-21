#include <arch/cpu.h>
#include <bits/errno.h>
#include <sys/thread.h>

tid_t thread_gettid(thread_t *thread) {
    return thread ? thread->t_tid : 0;
}

tid_t gettid(void) {
    return thread_gettid(current);
}