#include <arch/x86_64/mmu.h>
#include <arch/x86_64/system.h>
#include <arch/x86_64/thread.h>
#include <bits/errno.h>
#include <lib/stddef.h>
#include <lib/string.h>
#include <sys/system.h>
#include <sys/thread.h>

void arch_thread_exit(uintptr_t exit_code) {
    current_lock();
    current->t_exit = exit_code;
    current_enter_state(T_TERMINATED);
    sched();
    panic("thread: %d failed to zombie\n", current->t_tid);
    loop();
}

/// @brief all threads start here
static void arch_thread_start(void) {
    current_unlock();
}

/// @brief all threads end executution here.
static void arch_thread_stop(void) {
    uintptr_t rax = rdrax();
    arch_thread_exit(rax);
}

int arch_thread_init(x86_64_thread_t *thread, void *(*entry)(void *), void *arg) {
    tf_t *tf = NULL;
    context_t *ctx = NULL;
    uintptr_t *stack = NULL;

    if (!thread)
        return -EINVAL;

    stack = (uintptr_t *)ALIGN4K((thread->t_kstack + thread->t_kstacksz) - sizeof (thread_t));
    
    *--stack = (uintptr_t)arch_thread_stop;
    
    tf = (tf_t *)((uintptr_t)stack - sizeof *tf);
    
    tf->ss = SEG_KDATA64 << 3;
    tf->rbp = tf->rsp = (uintptr_t)stack;
    tf->rflags = LF_IF;
    tf->cs = SEG_KCODE64 << 3;
    tf->rip = (uintptr_t)entry;
    tf->rdi = (uintptr_t)arg;
    tf->fs = SEG_KDATA64 << 3;
    tf->ds = SEG_KDATA64 << 3;
    tf->rax = 0;

    stack = (uintptr_t *)tf;
    *--stack = (uintptr_t)trapret;
    ctx = (context_t *)((uintptr_t)stack - sizeof *ctx);
    ctx->rip = (uintptr_t)arch_thread_start;
    ctx->rbp = tf->rsp;

    thread->t_tf = tf;
    thread->t_ctx0 = ctx;

    return 0;
}