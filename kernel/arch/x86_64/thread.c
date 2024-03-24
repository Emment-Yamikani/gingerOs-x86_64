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

int x86_64_sighandler_init(arch_thread_t *thread, thread_entry_t entry, siginfo_t   *info, void *ucontext, sigaction_t *act) {
    int         err         = 0;
    tf_t        *tf         = NULL;
    context_t   *ctx        = NULL;
    ucontext_t  *uctx       = NULL;
    sig_stack_t *stack      = NULL;
    siginfo_t   *siginfo    = NULL;
    u64         *sig_stack  = NULL;

    if (!thread || !entry || !info || !act)
        return -EINVAL;
    
    if (NULL == (stack = (sig_stack_t *)kmalloc(sizeof *stack)))
        return -ENOMEM;

    stack->st_link = thread->t_sig_stack;
    
    if (!thread_isuser(thread->t_thread)) { // set up context of a user thread.
        if ((err = thread_kstack_alloc(KSTACKSZ, &sig_stack)))
            return err;

        stack->st_size  = KSTACKSZ;
        stack->st_addr  = (u64)sig_stack;
        
        sig_stack       = ALIGN4K((u64)sig_stack + KSTACKSZ);

        /** ucontext has to be set in case we get a signal chainning scenario*/
        sig_stack = (u64 *)(uctx= (ucontext_t *)((u64)sig_stack - sizeof *uctx));

        uctx->uc_link           = thread->ucontext;
        uctx->uc_sigmask        = 0;

        uctx->uc_stack.ss_flags = 0;
        uctx->uc_stack.ss_size  = KSTACKSZ;
        uctx->uc_stack.ss_sp    = sig_stack;

        uctx->uc_mcontext.r15   = thread->t_tf->r15;
        uctx->uc_mcontext.r14   = thread->t_tf->r14;
        uctx->uc_mcontext.r13   = thread->t_tf->r13;
        uctx->uc_mcontext.r12   = thread->t_tf->r12;
        uctx->uc_mcontext.r11   = thread->t_tf->r11;
        uctx->uc_mcontext.r10   = thread->t_tf->r10;
        uctx->uc_mcontext.r9    = thread->t_tf->r9;
        uctx->uc_mcontext.r8    = thread->t_tf->r8;
        uctx->uc_mcontext.rbp   = thread->t_tf->rbp;
        uctx->uc_mcontext.rsi   = thread->t_tf->rsi;
        uctx->uc_mcontext.rdi   = thread->t_tf->rdi;
        uctx->uc_mcontext.rdx   = thread->t_tf->rdx;
        uctx->uc_mcontext.rcx   = thread->t_tf->rcx;
        uctx->uc_mcontext.rbx   = thread->t_tf->rbx;
        uctx->uc_mcontext.rax   = thread->t_tf->rax;
        uctx->uc_mcontext.rsp   = thread->t_tf->rsp;
        uctx->uc_mcontext.rip   = thread->t_tf->rip;

        if (act->sa_flags & SA_SIGINFO) {
            sig_stack   = (u64 *)(siginfo = (siginfo_t *)((u64)sig_stack - sizeof *siginfo));
            *siginfo    = *info;
        }

        // set the return address of this signal handler.
        *--sig_stack = (u64)x86_64_signal_return;
        tf = (tf_t *)((u64)sig_stack - sizeof *tf);
        memset(tf, 0, sizeof *tf);

        tf->ss      = SEG_KDATA64 << 3;
        tf->rbp     = tf->rsp = (u64)sig_stack;
        tf->rflags  = LF_IF;
        tf->cs      = SEG_KCODE64 << 3;
        tf->rip     = (u64)entry;

        //////////////////////////////////////////////////////////
                    /* Pass the handler arguments */
        //////////////////////////////////////////////////////////
        tf->rdi     = (u64)info->si_signo;
        // Do we need to pass 2nd and 3rd arguments to sa_hander?
        if (act->sa_flags & SA_SIGINFO) {
            tf->rsi = (u64)siginfo;
            tf->rdx = (u64)uctx;
        }
        ///////////////////////////////////////////////////////////

        tf->fs      = SEG_KDATA64 << 3;
        tf->ds      = SEG_KDATA64 << 3;

    } else { // setup user thread for signal handling...

    }
    
    /** This is common both to a user and a kernel thread */

    sig_stack           = (u64 *)tf;
    *--sig_stack        = (u64)trapret;
    ctx                 = (context_t *)((u64)sig_stack - sizeof *ctx);
    ctx->rip            = (u64)x86_64_signal_start;
    ctx->rbp            = tf->rsp;
    thread->t_tf        = tf;
    thread->t_ctx0      = ctx;
    thread->t_sig_stack = stack;
    
    return 0;
}

int x86_64_uthread_init(arch_thread_t *thread, thread_entry_t entry, void *arg) {
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
    
    if (thread->t_kstack == 0 || thread->t_kstacksz)
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