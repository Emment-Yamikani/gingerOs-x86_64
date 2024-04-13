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
#include <arch/x86_64/system.h>

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

    // assert(0, current->t_arch.t_context);

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
    mcontext_t  *mctx   = NULL;
    context_t   *ctx    = NULL;
    u64         *kstack = NULL;

    if (!thread)
        return -EINVAL;

    kstack = (u64 *)ALIGN4K((thread->t_kstack.ss_sp +
        thread->t_kstack.ss_size) - sizeof (thread_t));

    *--kstack = (u64)x86_64_thread_stop;
    
    mctx = (mcontext_t *)((u64)kstack - sizeof *mctx);
    memset(mctx, 0, sizeof *mctx);
    
    mctx->ss      = SEG_KDATA64 << 3;
    mctx->rbp     = mctx->rsp = (u64)kstack;
    mctx->rflags  = LF_IF;
    mctx->cs      = SEG_KCODE64 << 3;
    mctx->rip     = (u64)entry;
    mctx->rdi     = (u64)arg;
    mctx->fs      = SEG_KDATA64 << 3;
    mctx->ds      = SEG_KDATA64 << 3;

    kstack        = (u64 *)mctx;
    *--kstack     = (u64)trapret;
    ctx           = (context_t *)((u64)kstack - sizeof *ctx);
    ctx->rip      = (u64)x86_64_thread_start;
    ctx->rbp      = mctx->rsp;
    ctx->link     = NULL; // starts with no link to old context.

    thread->t_context = ctx;
    return 0;
}

void x86_64_signal_return(void) {
    panic("%s()\n", __func__);
}

void x86_64_signal_start(void) {
    current_unlock();
}

int x86_64_signal_dispatch( arch_thread_t   *thread, thread_entry_t  entry,
    siginfo_t *info, sigaction_t *sigact, sigset_t sigmask
) {
    __unused int         err     = 0;
    __unused i64         ncli    = 0;
    __unused i64         intena  = 0;
    __unused context_t   *ctx    = NULL;
    __unused mcontext_t  *mctx   = NULL;
    __unused ucontext_t  *uctx   = NULL;
    __unused u64         *kstack = NULL;
    __unused u64         *ustack = NULL;

    (void)thread;
    (void)entry;
    (void)info;
    (void)sigact;
    (void)sigmask;

    if (thread == NULL || entry == NULL ||
        info   == NULL || sigact== NULL)
        return -EINVAL;
    
    if (thread->t_thread == NULL)
        return -EINVAL;


    ctx->link           = thread->t_context;
    thread->t_context   = ctx;

    current_setflags(THREAD_HANDLING_SIG);

    swapi64(&intena, &cpu->intena);
    swapi64(&ncli, &cpu->ncli);

    context_switch(&thread->t_context);

    swapi64(&cpu->ncli, &ncli);
    swapi64(&cpu->intena, &intena);

    current_maskflags(THREAD_HANDLING_SIG);
    return 0;
}

int x86_64_uthread_init(arch_thread_t *thread, thread_entry_t entry, void *arg) {
    int         err     = 0;
    mcontext_t  *mctx   = NULL;
    context_t   *ctx    = NULL;
    u64         *kstack = NULL;
    u64         *ustack = NULL;

    if (!thread)
        return -EINVAL;

    if ((ustack = (u64 *)(thread->t_ustack.ss_sp +
        thread->t_ustack.ss_size)) == NULL)
        return -EINVAL;

    if ((err = arch_map_n(((u64)ustack) -
        PGSZ, PGSZ, thread->t_ustack.ss_flags)))
        return err;

    kstack = (u64 *)ALIGN4K((thread->t_kstack.ss_sp +
        thread->t_kstack.ss_size) - sizeof(thread_t));

    *--kstack   = (u64)x86_64_thread_stop;
    *--ustack   = -1ull; // push dummy return address.

    mctx = (mcontext_t *)((u64)kstack - sizeof *mctx);
    memset(mctx, 0, sizeof *mctx);

    mctx->ss      = SEG_UDATA64 << 3 | DPL_USR;
    mctx->rbp     = mctx->rsp = (u64)ustack;
    mctx->rflags  = LF_IF;
    mctx->cs      = SEG_UCODE64 << 3 | DPL_USR;
    mctx->rip     = (u64)entry;
    mctx->rdi     = (u64)arg;

    mctx->fs      = SEG_UDATA64 << 3 | DPL_USR;
    mctx->ds      = SEG_UDATA64 << 3 | DPL_USR;

    kstack        = (u64 *)mctx;
    *--kstack     = (u64)trapret;
    ctx           = (context_t *)((u64)kstack - sizeof *ctx);
    ctx->rip      = (u64)x86_64_thread_start;
    ctx->rbp      = mctx->rsp;
    ctx->link     = NULL; // starts with no link to old context.

    thread->t_context   = ctx;
    return 0;
}

int x86_64_thread_execve(arch_thread_t *thread, thread_entry_t entry,
                       int argc, const char *argp[], const char *envp[]) {
    int err = 0;

    if (thread == NULL || entry == NULL)
        return -EINVAL;
    
    if (argc && argp == NULL)
        return -EINVAL;

    mcontext_t *mctx = NULL;
    u64 *ustack = NULL;
    context_t *ctx = NULL;
    u64 *kstack = NULL;

    if (!thread)
        return -EINVAL;

    if ((ustack = (u64 *)(thread->t_ustack.ss_sp +
        thread->t_ustack.ss_size)) == NULL)
        return -EINVAL;

    if ((err = arch_map_n(((u64)ustack) -
        PGSZ, PGSZ, thread->t_ustack.ss_flags)))
        return err;

    kstack = (u64 *)ALIGN4K((thread->t_kstack.ss_sp +
        thread->t_kstack.ss_size) - sizeof(thread_t));

    *--kstack = (u64)x86_64_thread_stop;
    *--ustack = -1ull; // push dummy return address.

    mctx = (mcontext_t *)((u64)kstack - sizeof *mctx);
    memset(mctx, 0, sizeof *mctx);

    mctx->ss      = SEG_UDATA64 << 3 | DPL_USR;
    mctx->rbp     = mctx->rsp = (u64)ustack;
    mctx->rflags  = LF_IF;
    mctx->cs      = SEG_UCODE64 << 3 | DPL_USR;
    mctx->rip     = (u64)entry;

    // pass paramenters to entry function.
    mctx->rdi     = (u64)argc;
    mctx->rsi     = (u64)argp;
    mctx->rdx     = (u64)envp;

    mctx->fs      = SEG_UDATA64 << 3 | DPL_USR;
    mctx->ds      = SEG_UDATA64 << 3 | DPL_USR;

    kstack        = (u64 *)mctx;
    *--kstack     = (u64)trapret;
    ctx           = (context_t *)((u64)kstack - sizeof *ctx);
    ctx->rip      = (u64)x86_64_thread_start;
    ctx->rbp      = mctx->rsp;
    ctx->link     = NULL; // starts with no link to old context.

    thread->t_context = ctx;
    return 0;
}

int x86_64_thread_setkstack(arch_thread_t *thread) {
    u64     kstack  = 0;
    if (thread == NULL)
        return -EINVAL;

    if ((thread->t_kstack.ss_sp == NULL) ||
        (thread->t_kstack.ss_size == 0))
        return -EFAULT;

    kstack = ALIGN4K((thread->t_kstack.ss_sp +
        thread->t_kstack.ss_size) - sizeof(thread_t));

    tss_set(kstack, SEG_KDATA64 << 3);
    return 0;
}

int x86_64_thread_fork(arch_thread_t *dst, arch_thread_t *src) {
    mcontext_t  *mctx   = NULL;
    context_t   *ctx    = NULL;
    u64         *kstack = NULL;

    if (dst == NULL || src == NULL)
        return -EINVAL;

    kstack = (u64 *)ALIGN4K((dst->t_kstack.ss_sp +
        dst->t_kstack.ss_size) - sizeof(thread_t));

    *--kstack = (u64)x86_64_thread_stop;

    mctx          = (mcontext_t *)(((u64)kstack) - sizeof *mctx);
    *mctx         = src->t_ucontext->uc_mcontext;
    mctx->rax     = 0;

    kstack      = (u64 *)mctx;
    *--kstack   = (u64)trapret;
    ctx         = (context_t *)(((u64)kstack) - sizeof *ctx);
    ctx->rip    = (u64)x86_64_thread_start;
    ctx->rbp    = ALIGN4K((dst->t_kstack.ss_sp +
        dst->t_kstack.ss_size) - sizeof(thread_t)
    );

    dst->t_context = ctx;
    return 0;
}

#endif // __x86_64__