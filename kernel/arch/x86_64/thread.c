#include <arch/x86_64/mmu.h>
#include <arch/x86_64/system.h>
#include <arch/thread.h>
#include <bits/errno.h>
#include <lib/stddef.h>
#include <lib/string.h>
#include <sys/system.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <arch/ucontext.h>

void x86_64_thread_exit(u64 exit_code) {
    current_lock();
    current_setflags(THREAD_EXITING);
    current->t_exit = exit_code;
    current_enter_state(T_TERMINATED);
    sched();
    panic(
        "thread: %d failed"
        " to zombie: flags: %X\n",
        thread_self(), current->t_flags
    );
    loop();
}

/// @brief all threads start here
static void x86_64_thread_start(void) {
    current_unlock();

    if (curproc) {
        // printk("%s(); pid=%d\n", __func__, curproc->pid);
        proc_lock(curproc);
        curproc->state = P_RUNNING;
        proc_unlock(curproc);
    }
}

/// @brief all threads end executution here.
static void x86_64_thread_stop(void) {
    u64 rax = rdrax();
    x86_64_thread_exit(rax);
}


/***********************************************************************************
 *                      x86_64 specific thread library functions.                  *
 * *********************************************************************************/
#if defined __x86_64__

int x86_64_kthread_init(arch_thread_t *thread, thread_entry_t entry, void *arg) {
    tf_t *tf = NULL;
    context_t *ctx = NULL;
    u64 *kstack = NULL;

    if (!thread)
        return -EINVAL;

    kstack = (u64 *)ALIGN4K((thread->t_kstack + thread->t_kstacksz) - sizeof (thread_t));
    
    *--kstack = (u64)x86_64_thread_stop;
    
    tf = (tf_t *)((u64)kstack - sizeof *tf);
    memset(tf, 0, sizeof *tf);
    
    tf->ss      = SEG_KDATA64 << 3;
    tf->rbp     = tf->rsp = (u64)kstack;
    tf->rflags  = LF_IF;
    tf->cs      = SEG_KCODE64 << 3;
    tf->rip     = (u64)entry;
    tf->rdi     = (u64)arg;
    tf->fs      = SEG_KDATA64 << 3;
    tf->ds      = SEG_KDATA64 << 3;

    kstack      = (u64 *)tf;
    *--kstack   = (u64)trapret;
    ctx         = (context_t *)((u64)kstack - sizeof *ctx);
    ctx->rip    = (u64)x86_64_thread_start;
    ctx->rbp    = tf->rsp;

    thread->t_tf = tf;
    thread->t_ctx0 = ctx;

    return 0;
}

void x86_64_signal_return(void) {
    panic("%s()\n", __func__);
}

void x86_64_signal_start(void) {
    current_unlock();
}

int x86_64_signal_init() {
    return -ENOSYS;
}

int x86_64_uthread_init(arch_thread_t *thread, thread_entry_t entry, void *arg) {
    int         err     = 0;
    tf_t        *tf     = NULL;
    context_t   *ctx    = NULL;
    u64         *kstack = NULL;
    u64         *ustack = NULL;

    if (!thread)
        return -EINVAL;

    if ((ustack = (u64 *)__vmr_upper_bound(thread->t_ustack)) == NULL)
        return -EINVAL;

    if ((err = arch_map_n(((u64)ustack) - PGSZ, PGSZ, thread->t_ustack->vflags)))
        return err;
    

    kstack = (u64 *)ALIGN4K((thread->t_kstack + thread->t_kstacksz) - sizeof(thread_t));

    *--kstack   = (u64)x86_64_thread_stop;
    *--ustack   = -1ull; // push dummy return address.

    tf = (tf_t *)((u64)kstack - sizeof *tf);
    memset(tf, 0, sizeof *tf);

    tf->ss      = SEG_UDATA64 << 3 | DPL_USR;
    tf->rbp     = tf->rsp = (u64)ustack;
    tf->rflags  = LF_IF;
    tf->cs      = SEG_UCODE64 << 3 | DPL_USR;
    tf->rip     = (u64)entry;
    tf->rdi     = (u64)arg;

    tf->fs      = SEG_UDATA64 << 3 | DPL_USR;
    tf->ds      = SEG_UDATA64 << 3 | DPL_USR;

    kstack      = (u64 *)tf;
    *--kstack   = (u64)trapret;
    ctx         = (context_t *)((u64)kstack - sizeof *ctx);
    ctx->rip    = (u64)x86_64_thread_start;
    ctx->rbp    = tf->rsp;

    thread->t_tf    = tf;
    thread->t_ctx0  = ctx;

    return 0;
}

int x86_64_uthread_signal_init(arch_thread_t *thread, thread_entry_t entry, void *arg) {
    int         err     = 0;
    tf_t        *tf     = NULL;
    context_t   *ctx    = NULL;
    u64   *kstack = NULL;
    u64   *ustack = NULL;

    if (!thread)
        return -EINVAL;

    if ((ustack = (u64 *)__vmr_upper_bound(thread->t_ustack)) == NULL)
        return -EINVAL;

    if ((err = arch_map_n(((u64)ustack) - PGSZ, PGSZ, thread->t_ustack->vflags)))
        return err;

    kstack = (u64 *)ALIGN4K((thread->t_kstack + thread->t_kstacksz) - sizeof(thread_t));

    *--kstack   = (u64)x86_64_thread_stop;
    *--ustack   = -1ull; // push dummy return address.

    tf = (tf_t *)((u64)kstack - sizeof *tf);
    memset(tf, 0, sizeof *tf);

    tf->ss      = SEG_UDATA64 << 3 | DPL_USR;
    tf->rbp     = tf->rsp = (u64)ustack;
    tf->rflags  = LF_IF;
    tf->cs      = SEG_UCODE64 << 3 | DPL_USR;
    tf->rip     = (u64)entry;
    tf->rdi     = (u64)arg;

    tf->fs      = SEG_UDATA64 << 3 | DPL_USR;
    tf->ds      = SEG_UDATA64 << 3 | DPL_USR;

    kstack      = (u64 *)tf;
    *--kstack   = (u64)trapret;
    ctx         = (context_t *)((u64)kstack - sizeof *ctx);
    ctx->rip    = (u64)x86_64_thread_start;
    ctx->rbp    = tf->rsp;

    thread->t_tf    = tf;
    thread->t_ctx0  = ctx;

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
    u64 *ustack = NULL;
    context_t *ctx = NULL;
    u64 *kstack = NULL;

    if (!thread)
        return -EINVAL;

    if ((ustack = (u64 *)__vmr_upper_bound(thread->t_ustack)) == NULL)
        return -EINVAL;

    if ((err = arch_map_n(((u64)ustack) - PGSZ, PGSZ, thread->t_ustack->vflags)))
        return err;

    kstack = (u64 *)ALIGN4K((thread->t_kstack + thread->t_kstacksz) - sizeof(thread_t));

    *--kstack = (u64)x86_64_thread_stop;
    *--ustack = -1ull; // push dummy return address.

    tf = (tf_t *)((u64)kstack - sizeof *tf);
    memset(tf, 0, sizeof *tf);

    tf->ss      = SEG_UDATA64 << 3 | DPL_USR;
    tf->rbp     = tf->rsp = (u64)ustack;
    tf->rflags  = LF_IF;
    tf->cs      = SEG_UCODE64 << 3 | DPL_USR;
    tf->rip     = (u64)entry;

    // pass paramenters to entry function.
    tf->rdi     = (u64)argc;
    tf->rsi     = (u64)argp;
    tf->rdx     = (u64)envp;

    tf->fs      = SEG_UDATA64 << 3 | DPL_USR;
    tf->ds      = SEG_UDATA64 << 3 | DPL_USR;

    kstack      = (u64 *)tf;
    *--kstack   = (u64)trapret;
    ctx         = (context_t *)((u64)kstack - sizeof *ctx);
    ctx->rip    = (u64)x86_64_thread_start;
    ctx->rbp    = tf->rsp;

    thread->t_tf = tf;
    thread->t_ctx0 = ctx;

    return 0;
}

int x86_64_thread_setkstack(arch_thread_t *thread) {
    u64 kstack = 0;
    if (thread == NULL)
        return -EINVAL;
    
    if (thread->t_kstack == 0 || thread->t_kstacksz == 0)
        return -EFAULT;
    
    kstack = ALIGN4K((thread->t_kstack + thread->t_kstacksz) - sizeof(thread_t));

    tss_set(kstack, SEG_KDATA64 << 3);
    
    return 0;
}

int x86_64_thread_fork(arch_thread_t *dst, arch_thread_t *src) {
    tf_t        *tf     = NULL;
    context_t   *ctx    = NULL;
    u64   *kstack = NULL;

    if (dst == NULL || src == NULL)
        return -EINVAL;

    kstack = (u64 *)ALIGN4K((dst->t_kstack + dst->t_kstacksz) - sizeof(thread_t));
    *--kstack = (u64)x86_64_thread_stop;

    tf          = (tf_t *)(((u64)kstack) - sizeof *tf);
    *tf         = *src->t_tf;
    tf->rax     = 0;

    kstack      = (u64 *)tf;
    *--kstack   = (u64)trapret;
    ctx         = (context_t *)(((u64)kstack) - sizeof *ctx);
    ctx->rip    = (u64)x86_64_thread_start;
    ctx->rbp    = ALIGN4K((dst->t_kstack + dst->t_kstacksz) - sizeof(thread_t));

    dst->t_tf   = tf;
    dst->t_ctx0 = ctx;

    return 0;
}

#endif // __x86_64__