#include <arch/x86_64/context.h>
#include <lib/printk.h>
#include <sys/system.h>
#include <lib/stddef.h>
#include <arch/x86_64/system.h>
#include <arch/traps.h>
#include <arch/lapic.h>
#include <arch/cpu.h>
#include <sys/thread.h>
#include <dev/clocks.h>
#include <arch/chipset.h>
#include <sys/_signal.h>
#include <dev/rtc.h>
#include <arch/x86_64/ipi.h>
#include <arch/paging.h>
#include <sys/syscall.h>
#include <sys/proc.h>

void dump_tf(mcontext_t *mctx, int halt) {
    void *stack_sp = NULL;
    usize stack_sz = 0;

    if (current) {
        stack_sp   = current->t_arch.t_kstack.ss_sp;
        stack_sz    = current->t_arch.t_kstack.ss_size;
    }

    if (halt) {
        panic("\n\e[025453;014mTRAP:%d\e[0m MCTX: %p CPU%d TID[%d:%d]\n"
              "\e[025453;015merr\e[0m=\e[025453;012m%16p\e[0m rfl=\e[025453;12m%16p\e[0m cs =\e[025453;12m%16p\e[0m\n"
              "\e[025453;015mds\e[0m =%16p \e[025453;015mfs \e[0m=%16p \e[025453;015mss \e[0m=%16p\n"
              "\e[025453;015mrax\e[0m=\e[025453;012m%16p\e[0m rbx=\e[025453;12m%16p\e[0m rcx=\e[025453;12m%16p\e[0m\n"
              "\e[025453;015mrdx\e[0m=%16p \e[025453;015mrdi\e[0m=%16p \e[025453;015mrsi\e[0m=%16p\n"
              "\e[025453;015mrbp\e[0m=\e[025453;012m%16p\e[0m rsp=\e[025453;12m%16p\e[0m r8 =\e[025453;12m%16p\e[0m\n"
              "\e[025453;015mr9\e[0m =%16p \e[025453;015mr10\e[0m=%16p \e[025453;015mr11\e[0m=%16p\n"
              "\e[025453;015mr12\e[0m=\e[025453;012m%16p\e[0m r13=\e[025453;12m%16p\e[0m r14=\e[025453;12m%16p\e[0m\n"
              "\e[025453;015mr15\e[0m=%16p \e[025453;015mrip\e[0m=%16p \e[025453;015mcr0\e[0m=%16p\n"
              "\e[025453;015mcr2\e[0m=\e[025453;012m%16p\e[0m cr3=\e[025453;12m%16p\e[0m cr4=\e[025453;12m%16p\e[0m\n"
              "\e[025453;015mst\e[0m =%16p \e[025453;015msp\e[0m =%16p \e[025453;015mstz\e[0m=%16p\n",
              mctx->trapno, mctx, getcpuid(), curproc ? curproc->pid : -1, thread_self(),
              mctx->errno,  mctx->rflags, mctx->cs,
              mctx->ds,     mctx->fs,     mctx->ss,
              mctx->rax,    mctx->rbx,    mctx->rcx,
              mctx->rdx,    mctx->rdi,    mctx->rsi,
              mctx->rbp,    mctx->rsp,    mctx->r8,
              mctx->r9,     mctx->r10,    mctx->r11,
              mctx->r12,    mctx->r13,    mctx->r14,
              mctx->r15,    mctx->rip,    rdcr0(),
              rdcr2(),      rdcr3(),      rdcr4(),
              stack_sp, stack_sp + stack_sz, stack_sz
        );
    } else {
        printk("\n\e[025453;014mTRAP:%d\e[0m MCTX: %p CPU%d TID[%d:%d]\n"
              "\e[025453;015merr\e[0m=\e[025453;012m%16p\e[0m rfl=\e[025453;12m%16p\e[0m cs =\e[025453;12m%16p\e[0m\n"
              "\e[025453;015mds\e[0m =%16p \e[025453;015mfs \e[0m=%16p \e[025453;015mss \e[0m=%16p\n"
              "\e[025453;015mrax\e[0m=\e[025453;012m%16p\e[0m rbx=\e[025453;12m%16p\e[0m rcx=\e[025453;12m%16p\e[0m\n"
              "\e[025453;015mrdx\e[0m=%16p \e[025453;015mrdi\e[0m=%16p \e[025453;015mrsi\e[0m=%16p\n"
              "\e[025453;015mrbp\e[0m=\e[025453;012m%16p\e[0m rsp=\e[025453;12m%16p\e[0m r8 =\e[025453;12m%16p\e[0m\n"
              "\e[025453;015mr9\e[0m =%16p \e[025453;015mr10\e[0m=%16p \e[025453;015mr11\e[0m=%16p\n"
              "\e[025453;015mr12\e[0m=\e[025453;012m%16p\e[0m r13=\e[025453;12m%16p\e[0m r14=\e[025453;12m%16p\e[0m\n"
              "\e[025453;015mr15\e[0m=%16p \e[025453;015mrip\e[0m=%16p \e[025453;015mcr0\e[0m=%16p\n"
              "\e[025453;015mcr2\e[0m=\e[025453;012m%16p\e[0m cr3=\e[025453;12m%16p\e[0m cr4=\e[025453;12m%16p\e[0m\n"
              "\e[025453;015mst\e[0m =%16p \e[025453;015msp\e[0m =%16p \e[025453;015mstz\e[0m=%16p\n",
              mctx->trapno, mctx, getcpuid(), curproc ? curproc->pid : -1, thread_self(),
              mctx->errno,  mctx->rflags, mctx->cs,
              mctx->ds,     mctx->fs,     mctx->ss,
              mctx->rax,    mctx->rbx,    mctx->rcx,
              mctx->rdx,    mctx->rdi,    mctx->rsi,
              mctx->rbp,    mctx->rsp,    mctx->r8,
              mctx->r9 ,    mctx->r10,    mctx->r11,
              mctx->r12,    mctx->r13,    mctx->r14,
              mctx->r15,    mctx->rip,    rdcr0(),
              rdcr2(),      rdcr3(),      rdcr4(),
              stack_sp, stack_sp + stack_sz, stack_sz
        );
    }
}

void dump_ctx(context_t *ctx, int halt) {
    if (halt) {
        panic(
            "\nctx: %p lnk: %p r11: %p\n"
            "r12: %p r13: %p r14: %p\n"
            "r15: %p rbp: %p rbx: %p rip: %p\n",
            ctx, ctx->link, ctx->r11,
            ctx->r12, ctx->r13, ctx->r14,
            ctx->r15, ctx->rbp, ctx->rbx, ctx->rip
        );
    } else {
        printk(
            "\nctx: %p lnk: %p r11: %p\n"
            "r12: %p r13: %p r14: %p\n"
            "r15: %p rbp: %p rbx: %p rip: %p\n",
            ctx, ctx->link, ctx->r11,
            ctx->r12, ctx->r13, ctx->r14,
            ctx->r15, ctx->rbp, ctx->rbx, ctx->rip
        );
    }
}

static void thread_handle_event(ucontext_t *uctx) {
    jiffies_t time = 0;

    if (current_iskilled())
        thread_exit(-EINTR);

    pushcli();
    if ((current_isuser() && uctx_isuser(uctx)) || !current_isuser())
        signal_dispatch();
    popcli();

    current_lock();
    time = current->t_sched.ts_timeslice;
    if (current_testflags(THREAD_STOP))
        thread_stop(current, sched_stopq);
    current_unlock();

    if (time <= 0)
        thread_yield();

    if (current_iskilled())
        thread_exit(-EINTR);
}

void trap(ucontext_t *uctx) {
    arch_thread_t   *arch   = NULL;
    mcontext_t      *mctx   = &uctx->uc_mcontext;

    if (current) {
        arch                = &current->t_arch;
        uctx->uc_stack      = current_isuser() ?
            arch->t_kstack : arch->t_ustack;
        pushcli();
        uctx->uc_link       = arch->t_uctx;
        arch->t_uctx        = uctx;
        popcli();
        uctx->uc_flags      = 0;
        sigemptyset(&uctx->uc_sigmask);
    }

    switch (mctx->trapno) {
    case T_LEG_SYSCALL:
        thread_handle_event(uctx);
        do_syscall(uctx);
        break;
    case IRQ(0):
        pit_intr();
        lapic_eoi();
        break;
    case IRQ(HPET):
        timer_intr();
        lapic_eoi();
        break;
    case T_FPU_NM:
        coprocessor_except();
        break;
    case T_SIMD_XM:
        simd_fp_except();
        break;
    case TLB_SHTDWN:
        tlb_shootdown_handler();
        lapic_eoi();
        break;
    case LAPIC_ERROR:
        lapic_eoi();
        break;
    case IRQ(IRQ_RTC):
        rtc_intr();
        lapic_eoi();
        break;
    case IRQ(7):
        __fallthrough;
    case LAPIC_SPURIOUS:
        lapic_eoi();
        break;
    case LAPIC_TIMER:
        lapic_timerintr();
        lapic_eoi();
        break;
    case T_PGFAULT:
        pushcli();
        arch_do_page_fault(mctx);
        lapic_eoi();
        popcli();
        break;
    default:
        dump_tf(mctx, 1);
        break;
    }

    // sched_remove_zombies();

    if (!current)
        return;

    thread_handle_event(uctx);

    arch->t_uctx = uctx->uc_link;
}