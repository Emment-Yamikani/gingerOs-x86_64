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

    mctx = (mcontext_t *)ALIGN16(thread->t_sstack.ss_sp - sizeof *mctx);
    memset(mctx, 0, sizeof *mctx);

    kstack      = (u64 *)ALIGN16(thread->t_rsvd);
    *--kstack   = (u64)x86_64_thread_stop;
    
    mctx->ss    = (SEG_KDATA64 << 3);
    mctx->rbp   = (u64)kstack;
    mctx->rsp   = (u64)kstack;
    mctx->rflags= LF_IF;
    mctx->cs    = (SEG_KCODE64 << 3);
    mctx->rip   = (u64)entry;
    mctx->rdi   = (u64)arg;
    mctx->fs    = (SEG_KDATA64 << 3);
    mctx->ds    = (SEG_KDATA64 << 3);

    kstack      = (u64 *)mctx;
    *--kstack   = (u64)trapret;
    ctx         = (context_t *)((u64)kstack - sizeof *ctx);
    ctx->rip    = (u64)x86_64_thread_start;
    ctx->rbp    = mctx->rsp;
    ctx->link   = NULL; // starts with no link to old context.

    thread->t_ctx = ctx;
    return 0;
}

void x86_64_signal_return(void) {
    context_t       *scheduler      = NULL;
    context_t       *dispatcher     = NULL;
    arch_thread_t   *arch           = NULL;

    current_lock();
    arch        = &current->t_arch;
    scheduler   = arch->t_ctx;              // schedule();
    dispatcher  = scheduler->link;      // signal_handler();
    
    scheduler->link = dispatcher->link;
    dispatcher->link= scheduler;
    arch->t_ctx     = dispatcher;

    context_switch(&arch->t_ctx);
}

void x86_64_signal_start(u64 *kstack, mcontext_t *mctx) {
    context_t       *scheduler      = NULL;
    context_t       *dispatcher     = NULL;
    arch_thread_t   *arch           = &current->t_arch;

    /**
     * Swap contexts otherwise context_switch()
     * in sched() will exit
     * the signal handler prematurely.
     *
     * Therefore by doing this we intend for
     * sched() to return to the context prior to
     * acquisition on this signal(i.e the scheduler' context)
     * Thus the thread will execute normally
     * as though it's not handling any signal.
     */

    dispatcher  = arch->t_ctx;      // signal_handler();
    scheduler   = dispatcher->link; // schedule();
    
    dispatcher->link    =  scheduler->link;
    scheduler->link     = dispatcher;

    arch->t_ctx         = scheduler;
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
    i64         ncli            = 1;
    i64         intena          = 0;
    flags64_t   was_handling    = 0;
    uc_stack_t  stack           = {0};
    context_t   *ctx            = NULL;
    mcontext_t  *mctx           = NULL;
    ucontext_t  *uctx           = NULL;
    ucontext_t  *tmpctx         = NULL;
    u64         *kstack         = NULL;
    u64         *ustack         = NULL;
    siginfo_t   *siginfo        = NULL;
    void        *rsvd_stack     = NULL;

    if (thread == NULL || entry == NULL ||
        info   == NULL || sigact== NULL)
        return -EINVAL;

    if (thread->t_thread == NULL)
        return -EINVAL;

    mctx = (mcontext_t *)(thread->t_sstack.ss_sp - sizeof *mctx);
    memset(mctx, 0, sizeof *mctx);

    if (current_isuser()) {
        if (sigact->sa_flags & SA_ONSTACK) {
            stack   = thread->t_altstack;
            stack.ss_sp = ((thread->t_flags & ARCH_EXEC_ONSTACK) ?
                (void *)ALIGN16(thread->t_uctx->uc_mcontext.rsp) :
                thread->t_altstack.ss_sp);
            stack.ss_size = thread->t_altstack.ss_size - 
                (thread->t_altstack.ss_sp - stack.ss_sp);
            stack.ss_flags = thread->t_altstack.ss_flags;
        } else {
            stack.ss_sp     = (void *)ALIGN16(thread->t_uctx->uc_mcontext.rsp);
            stack.ss_size   = thread->t_ustack.ss_size - 
                (thread->t_ustack.ss_sp - stack.ss_sp);
            stack.ss_flags  = thread->t_ustack.ss_flags;
        }

        ustack  = stack.ss_sp;

        if (sigact->sa_flags & SA_SIGINFO) {
            for (tmpctx = thread->t_uctx; tmpctx; tmpctx = tmpctx->uc_link)
                ustack  = (u64 *)((u64)ustack - sizeof *uctx);

            uctx        = (ucontext_t *)ustack;

            assert(((void *)uctx - 0) >= 
                (stack.ss_sp - stack.ss_size),
                "User stack overflow detected!!!\n"
            );

            for (tmpctx = thread->t_uctx; tmpctx; tmpctx = tmpctx->uc_link) {
                uctx->uc_flags      = tmpctx->uc_flags;
                uctx->uc_resvd      = tmpctx->uc_resvd;
                uctx->uc_stack      = tmpctx->uc_stack;
                uctx->uc_sigmask    = tmpctx->uc_sigmask;
                uctx->uc_mcontext   = tmpctx->uc_mcontext;
                uctx = uctx->uc_link= tmpctx->uc_link ? (uctx + 1) : NULL;
            }

            uctx        = (ucontext_t *)ustack;
            siginfo     = (siginfo_t *)((u64)ustack - sizeof *siginfo);

            assert(((void *)siginfo - 0) >= 
                (stack.ss_sp - stack.ss_size),
                "User stack overflow detected!!!\n"
            );

            siginfo->si_pid     = info->si_pid;
            siginfo->si_uid     = info->si_uid;
            siginfo->si_addr    = info->si_addr;
            siginfo->si_code    = info->si_code;
            siginfo->si_value   = info->si_value;
            siginfo->si_signo   = info->si_signo;
            siginfo->si_status  = info->si_status;

            ustack      = (u64 *)siginfo;
        }

        *--ustack   = -1ull;
    
        assert(((void *)ustack - 0) >= 
            (stack.ss_sp - stack.ss_size),
            "User stack overflow detected!!!\n"
        );

        mctx->ss    = (SEG_UDATA64 << 3) | DPL_USR;
        mctx->rbp   = (u64)ustack;
        mctx->rsp   = (u64)ustack;
        mctx->cs    = (SEG_UCODE64 << 3) | DPL_USR;
        mctx->fs    = (SEG_UDATA64 << 3) | DPL_USR;
        mctx->ds    = (SEG_UDATA64 << 3) | DPL_USR;
    } else {
        mctx->ss = (SEG_KDATA64 << 3);
        mctx->cs = (SEG_KCODE64 << 3);
        mctx->ds = (SEG_KDATA64 << 3);
        mctx->fs = (SEG_KDATA64 << 3);
        /**
         * @brief mctx->rsp and mctx->rbp
         * are set in x86_64_signal_start() above.
         */
    }

    mctx->rflags    = LF_IF;
    mctx->rip       = (u64)entry;
    mctx->rdi       = (u64)info->si_signo;
    
    if (sigact->sa_flags & SA_SIGINFO) {
        mctx->rsi   = (u64) (current_isuser() ? siginfo : info);
        mctx->rdx   = (u64) (current_isuser() ? uctx : thread->t_uctx);
    }

    kstack          = (u64 *)mctx;
    *--kstack       = (u64)trapret;
    ctx             = (context_t *)((u64)kstack - sizeof *ctx);
    ctx->rip        = (u64)x86_64_signal_start;
    ctx->rbp        = (u64)kstack;
    ctx->link       = thread->t_ctx;
    thread->t_ctx   = ctx;

    rsvd_stack      = thread->t_rsvd;
    was_handling    = current_ishandling();
    current_setflags(THREAD_HANDLING_SIG);

    swapi64(&intena, &cpu->intena);
    swapi64(&ncli, &cpu->ncli);

    context_switch(&thread->t_ctx);

    swapi64(&cpu->ncli, &ncli);
    swapi64(&cpu->intena, &intena);

    if (was_handling == 0)
        current_maskflags(THREAD_HANDLING_SIG);

    /**
     * thread->t_ctx at this point points to
     * context_t saved by calling context_switch()
     * in x86_64_signal_return(): see above.
     * 
     * Therefore, we need to restore thread->t_ctx
     * to the value it had prior to calling
     * this functino (x86_64_signal_dispatch()).
    */
    thread->t_ctx   = thread->t_ctx->link;
    thread->t_rsvd  = rsvd_stack;

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

    if ((ustack = (u64 *)ALIGN16(thread->t_ustack.ss_sp)) == NULL)
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

    mctx->ss      = (SEG_UDATA64 << 3) | DPL_USR;
    mctx->rbp     = mctx->rsp = (u64)ustack;
    mctx->rflags  = LF_IF;
    mctx->cs      = (SEG_UCODE64 << 3) | DPL_USR;
    mctx->rip     = (u64)entry;
    mctx->rdi     = (u64)arg;

    mctx->fs      = (SEG_UDATA64 << 3) | DPL_USR;
    mctx->ds      = (SEG_UDATA64 << 3) | DPL_USR;

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
    int         err     = 0;
    mcontext_t  *mctx   = NULL;
    u64         *ustack = NULL;
    context_t   *ctx    = NULL;
    u64         *kstack = NULL;

    if (thread == NULL || entry == NULL)
        return -EINVAL;

    if ((ustack = (u64 *)ALIGN16(thread->t_ustack.ss_sp)) == NULL)
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

    mctx->ss      = (SEG_UDATA64 << 3) | DPL_USR;
    mctx->rbp     = mctx->rsp = (u64)ustack;
    mctx->rflags  = LF_IF;
    mctx->cs      = (SEG_UCODE64 << 3) | DPL_USR;
    mctx->rip     = (u64)entry;

    // pass paramenters to entry function.
    mctx->rdi     = (u64)argc;
    mctx->rsi     = (u64)argp;
    mctx->rdx     = (u64)envp;

    mctx->fs      = (SEG_UDATA64 << 3) | DPL_USR;
    mctx->ds      = (SEG_UDATA64 << 3) | DPL_USR;

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