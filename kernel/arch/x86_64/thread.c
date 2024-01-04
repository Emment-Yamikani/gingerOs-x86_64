#include <arch/x86_64/mmu.h>
#include <arch/x86_64/system.h>
#include <arch/thread.h>
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


/***********************************************************************************
 *                      x86_64 specific thread library functions.                  *
 * *********************************************************************************/
#if defined __x86_64__

    int
    x86_64_kthread_init(arch_thread_t *thread, thread_entry_t entry, void *arg) {
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

int x86_64_uthread_init(arch_thread_t *thread, thread_entry_t entry, void *arg) {
    tf_t *tf = NULL;
    context_t *ctx = NULL;
    uintptr_t *stack = NULL;

    if (!thread)
        return -EINVAL;

    stack = (uintptr_t *)ALIGN4K((thread->t_kstack + thread->t_kstacksz) - sizeof(thread_t));

    *--stack = (uintptr_t)arch_thread_stop;

    tf = (tf_t *)((uintptr_t)stack - sizeof *tf);

    tf->ss = SEG_UDATA64 << 3 | DPL_USR;
    tf->rbp = tf->rsp = (uintptr_t)stack;
    tf->rflags = LF_IF;
    tf->cs = SEG_UCODE64 << 3 | DPL_USR;
    tf->rip = (uintptr_t)entry;
    tf->rdi = (uintptr_t)arg;
    tf->fs = SEG_UDATA64 << 3 | DPL_USR;
    tf->ds = SEG_UDATA64 << 3 | DPL_USR;
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

int x86_64_thread_execve(arch_thread_t *thread, thread_entry_t entry,
                       int argc, const char *argp[], const char *envp[]) {
    int err = 0;

    if (thread == NULL || entry == NULL)
        return -EINVAL;
    
    if (argc && argp == NULL)
        return -EINVAL;

    tf_t *tf = NULL;
    uintptr_t *ustack = NULL;
    context_t *ctx = NULL;
    uintptr_t *stack = NULL;

    if (!thread)
        return -EINVAL;

    ustack = (uintptr_t *)__vmr_upper_bound(thread->t_ustack);

    if ((err = arch_map_n(((uintptr_t)ustack) - PGSZ, PGSZ, thread->t_ustack->vflags)))
        return err;

    stack = (uintptr_t *)ALIGN4K((thread->t_kstack + thread->t_kstacksz) - sizeof(thread_t));

    *--stack = (uintptr_t)arch_thread_stop;
    *--ustack = -1;

    tf = (tf_t *)((uintptr_t)stack - sizeof *tf);

    tf->ss = SEG_UDATA64 << 3 | DPL_USR;
    tf->rbp = tf->rsp = (uintptr_t)ustack;
    tf->rflags = LF_IF;
    tf->cs = SEG_UCODE64 << 3 | DPL_USR;
    tf->rip = (uintptr_t)entry;

    // pass paramenters to entry function.
    tf->rdi = (uintptr_t)argc;
    tf->rsi = (uintptr_t)argp;
    tf->rdx = (uintptr_t)envp;

    tf->fs = SEG_UDATA64 << 3 | DPL_USR;
    tf->ds = SEG_UDATA64 << 3 | DPL_USR;
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

int x86_64_thread_setkstack(arch_thread_t *arch) {
    uintptr_t kstack = 0;
    if (arch == NULL)
        return -EINVAL;
    
    if (arch->t_kstack == 0 || arch->t_sig_kstacksz)
        return -EFAULT;
    
    kstack = ALIGN4K((arch->t_kstack + arch->t_kstacksz) - sizeof(thread_t));

    tss_set(kstack, SEG_KDATA64 << 3);
    
    return 0;
}

#endif // __x86_64__