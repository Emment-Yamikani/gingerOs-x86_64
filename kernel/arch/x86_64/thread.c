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

    // assert(0, current->t_arch.t_ctx);

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
    context_t   *ctx    = NULL;
    mcontext_t  *mctx   = NULL;
    u64         *kstack = NULL;

    if (thread == NULL || entry == NULL)
        return -EINVAL;

    mctx = (mcontext_t *)(thread->t_sstack.ss_sp - sizeof *mctx);
    memset(mctx, 0, sizeof *mctx);

    kstack = (u64 *)thread->t_rsvd;
    assert(kstack, "Invalid Kernel stack.");
    assert(is_aligned16(kstack),
           "Kernel stack is not 16bytes aligned!");
    *--kstack = (u64)x86_64_thread_stop;
    
    mctx->ss      = (SEG_KDATA64 << 3);
    mctx->rbp     = mctx->rsp = (u64)kstack;
    mctx->rflags  = LF_IF;
    mctx->cs      = (SEG_KCODE64 << 3);
    mctx->rip     = (u64)entry;
    mctx->rdi     = (u64)arg;
    mctx->fs      = (SEG_KDATA64 << 3);
    mctx->ds      = (SEG_KDATA64 << 3);

    kstack        = (u64 *)mctx;
    *--kstack     = (u64)trapret;
    ctx           = (context_t *)((u64)kstack - sizeof *ctx);
    ctx->rip      = (u64)x86_64_thread_start;
    ctx->rbp      = mctx->rsp;
    ctx->link     = NULL; // starts with no link to old context.

    thread->t_ctx = ctx;
    return 0;
}

void x86_64_signal_return(void) {
    context_t       *scheduler_ctx      = NULL;
    context_t       *signal_handler_ctx = NULL;
    arch_thread_t   *arch               = NULL;

    current_lock();
    arch = &current->t_arch;

    /**
     * Swap contexts otherwise context_switch()
     * in sched() will exit
     * the signal handler prematurely.
     *
     * Therefore by doing this we intend for
     * sched() to return to the context prior to
     * acquisition on this signal(i.e the scheduler_ctx' context)
     * Thus the thread will execute normally
     * as though it's not handling any signal.
     */

    scheduler_ctx       = arch->t_ctx; // schedule();
    signal_handler_ctx  = scheduler_ctx->link;      // signal_handler();
    
    scheduler_ctx->link = signal_handler_ctx->link;
    signal_handler_ctx->link = scheduler_ctx;
    arch->t_ctx         = signal_handler_ctx;

    context_switch(&arch->t_ctx);
}

void x86_64_signal_start(u64 *kstack, mcontext_t *mctx) {
    context_t       *scheduler_ctx      = NULL;
    context_t       *signal_handler_ctx = NULL;
    arch_thread_t   *arch               = &current->t_arch;

    /**
     * Swap contexts otherwise context_switch()
     * in sched() will exit
     * the signal handler prematurely.
     *
     * Therefore by doing this we intend for
     * sched() to return to the context prior to
     * acquisition on this signal(i.e the scheduler_ctx' context)
     * Thus the thread will execute normally
     * as though it's not handling any signal.
     */


    signal_handler_ctx  = arch->t_ctx;      // signal_handler();
    scheduler_ctx       = signal_handler_ctx->link; // schedule();
    
    signal_handler_ctx->link =  scheduler_ctx->link;
    scheduler_ctx->link = signal_handler_ctx;

    // printk(
        // "kstack: %p\nmctx:   %p\n"
        // "ctx:    %p\nuctx:   %p\n",
        // kstack, mctx, arch->t_ctx, arch->t_uctx
    // );
    arch->t_ctx         = scheduler_ctx;
    assert((void *)kstack <= (void *)mctx, "Stack overun detected!");

    kstack = (void *)ALIGN16(kstack);
    current->t_arch.t_rsvd = kstack;
    assert(is_aligned16(kstack),
           "Kernel stack is not 16bytes aligned!");

    *--kstack = (u64)x86_64_signal_return;

    if (!current_isuser())
        mctx->rsp = mctx->rbp = (u64)kstack;
    else
        x86_64_thread_setkstack(&current->t_arch);

    current_unlock();
}

int x86_64_signal_dispatch( arch_thread_t   *thread, thread_entry_t  entry,
    siginfo_t *info, sigaction_t *sigact) {
    __unused int         err     = 0;
    i64         ncli    = 1;
    i64         intena  = 0;
    context_t   *ctx    = NULL;
    mcontext_t  *mctx   = NULL;
    __unused ucontext_t  *uctx   = NULL;
    u64         *kstack = NULL;
    __unused u64         *ustack = NULL;
    flags64_t   was_handling   = 0;
    void        *rsvd_stack = NULL;

    if (thread == NULL || entry == NULL ||
        info   == NULL || sigact== NULL)
        return -EINVAL;
    
    if (thread->t_thread == NULL)
        return -EINVAL;

    kstack = (u64 *)thread->t_sstack.ss_sp;
    assert(kstack, "Invalid Kernel stack.");
    assert(is_aligned16(kstack),
           "Kernel stack is not 16bytes aligned!");

    *--kstack = (u64)x86_64_signal_return;
    mctx = (mcontext_t *)((u64)kstack - sizeof *mctx);
    memset(mctx, 0, sizeof *mctx);

    mctx->rflags    = LF_IF;
    mctx->rip       = (u64)entry;

    if (!current_isuser()) {
        mctx->ss    = (SEG_KDATA64 << 3);
        mctx->rbp   = mctx->rsp = (u64)kstack;
        mctx->cs    = (SEG_KCODE64 << 3);
        mctx->fs    = (SEG_KDATA64 << 3);
        mctx->ds    = (SEG_KDATA64 << 3);
    } else {

    } 

    // first argument of signal handler.
    mctx->rdi   = (u64)info->si_signo;
    //  Pass 2nd and 3rd argument of sa_sigaction(); ?
    if (sigact->sa_flags & SA_SIGINFO) {
        /**
         * If this is a kernel thread
         * we don't need any special manipulations
         * Just pass the two arguments as they are
         * since these are kernel pointers.
         * so that shouldn't be a problem.
        */
        if (!current_isuser()) {
            mctx->rsi   = (u64)info;
            mctx->rdx   = (u64)thread->t_uctx;
        } else { // this is a user thread and need special care.

        }
    }

    kstack      = (u64 *)mctx;
    *--kstack   = (u64)trapret;
    ctx         = (context_t *)((u64)kstack - sizeof *ctx);

    assert((void *)ctx > (thread->t_sstack.ss_sp -
                          thread->t_sstack.ss_size),
           "Kernel stack overflow detected!");

    ctx->rip    = (u64)x86_64_signal_start;
    ctx->rbp    = mctx->rsp;
    ctx->link   = thread->t_ctx; // -> interrupted ctx.
    thread->t_ctx   = ctx;

    was_handling = current_ishandling();
    current_setflags(THREAD_HANDLING_SIG);

    swapi64(&intena, &cpu->intena);
    swapi64(&ncli, &cpu->ncli);

    rsvd_stack = thread->t_rsvd;

    context_switch(&thread->t_ctx);

    /**
     * thread->t_ctx at this point points to
     * context_t saved by calling context_switch()
     * in x86_64_signal_return(): see above.
     * 
     * Therefore we need to restore thread->t_ctx
     * to the value it had prior to calling
     * this functino (x86_64_signal_dispatch()).
    */
    thread->t_ctx = thread->t_ctx->link;

    thread->t_rsvd = rsvd_stack;

    swapi64(&cpu->ncli, &ncli);
    swapi64(&cpu->intena, &intena);

    if (was_handling == 0)
        current_maskflags(THREAD_HANDLING_SIG);

    return 0;
}

int x86_64_uthread_init(arch_thread_t *thread, thread_entry_t entry, void *arg) {
    int         err     = 0;
    context_t   *ctx    = NULL;
    mcontext_t  *mctx   = NULL;
    u64         *kstack = NULL;
    u64         *ustack = NULL;

    if (thread == NULL || entry == NULL)
        return -EINVAL;

    if ((ustack = (u64 *)(thread->t_ustack.ss_sp +
        thread->t_ustack.ss_size)) == NULL)
        return -EINVAL;

    if ((err = arch_map_n(((u64)ustack) -
        PGSZ, PGSZ, thread->t_ustack.ss_flags)))
        return err;

    kstack = (u64 *)thread->t_sstack.ss_sp;
    assert(kstack, "Invalid Kernel stack.");
    assert(is_aligned16(kstack),
           "Kernel stack is not 16bytes aligned!");

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

    thread->t_ctx   = ctx;
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

    if (thread == NULL || entry == NULL)
        return -EINVAL;

    if ((ustack = (u64 *)(thread->t_ustack.ss_sp +
        thread->t_ustack.ss_size)) == NULL)
        return -EINVAL;

    if ((err = arch_map_n(((u64)ustack) -
        PGSZ, PGSZ, thread->t_ustack.ss_flags)))
        return err;

    kstack = (u64 *)thread->t_sstack.ss_sp;
    assert(kstack, "Invalid Kernel stack.");
    assert(is_aligned16(kstack),
           "Kernel stack is not 16bytes aligned!");

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

    thread->t_ctx = ctx;
    return 0;
}

int x86_64_thread_setkstack(arch_thread_t *thread) {
    u64     kstack  = 0;
    if (thread == NULL)
        return -EINVAL;

    if (thread->t_rsvd < thread->t_kstack.ss_sp)
        return -EOVERFLOW;

    kstack = (u64)thread->t_rsvd;
    tss_set(kstack, (SEG_KDATA64 << 3));
    return 0;
}

int x86_64_thread_fork(arch_thread_t *dst, arch_thread_t *src) {
    mcontext_t  *mctx   = NULL;
    context_t   *ctx    = NULL;
    u64         *kstack = NULL;

    if (dst == NULL || src == NULL)
        return -EINVAL;

    kstack = (u64 *)dst->t_sstack.ss_sp;
    assert(kstack, "Invalid Kernel stack.");
    assert(is_aligned16(kstack),
           "Kernel stack is not 16bytes aligned!");

    *--kstack = (u64)x86_64_thread_stop;

    mctx        = (mcontext_t *)(((u64)kstack) - sizeof *mctx);
    *mctx       = src->t_uctx->uc_mcontext;
    mctx->rax   = 0;

    kstack      = (u64 *)mctx;
    *--kstack   = (u64)trapret;
    ctx         = (context_t *)(((u64)kstack) - sizeof *ctx);
    ctx->rip    = (u64)x86_64_thread_start;
    ctx->rbp    = (u64)dst->t_rsvd;
    ctx->link   = NULL;
    dst->t_ctx  = ctx;
    return 0;
}

#endif // __x86_64__