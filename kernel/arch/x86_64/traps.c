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

void dump_tf(mcontext_t *mctx, int halt) {
    if (halt) {
        panic(
            "\n\e[025453;014mTRAP:%d\e[0m CPU%d TID:%d: ERR:%X rflags=%8X cs=%X ds=%X fs=%X ss=%X\n"
            "\e[025453;015mrax\e[0m=\e[025453;016m%16p\e[0m \e[025453;015mrbx\e[0m=\e[025453;12m%16p\e[0m \e[025453;015mrcx\e[0m=\e[025453;12m%16p\n\e[0m"
            "\e[025453;015mrdx\e[0m=%16p \e[025453;015mrdi\e[0m=%16p \e[025453;015mrsi\e[0m=%16p\n"
            "\e[025453;03mrbp\e[0m=\e[025453;03m%16p\e[0m \e[025453;03mrsp\e[0m=\e[025453;03m%16p\e[0m \e[025453;015mr8\e[0m =\e[025453;12m%16p\n\e[0m"
            "\e[025453;015mr9\e[0m =%16p \e[025453;015mr10\e[0m=%16p \e[025453;015mr11\e[0m=%16p\n"
            "\e[025453;015mr12\e[0m=\e[025453;012m%16p\e[0m \e[025453;015mr13\e[0m=\e[025453;12m%16p\e[0m \e[025453;015mr14\e[0m=\e[025453;12m%16p\n\e[0m"
            "\e[025453;015mr15\e[0m=%16p \e[025453;015mrip\e[0m=\e[025453;016m%16p\e[0m \e[025453;015mcr0\e[0m=%16p\n"
            "\e[025453;015mcr2\e[0m=\e[025453;016m%16p\e[0m \e[025453;015mcr3\e[0m=\e[025453;12m%16p\e[0m \e[025453;015mcr4\e[0m=\e[025453;12m%16p\n\e[0m",
            mctx->trapno, getcpuid(), thread_self(), mctx->errno, mctx->rflags, mctx->cs, mctx->ds, mctx->fs, mctx->ss, 
            mctx->rax, mctx->rbx, mctx->rcx,
            mctx->rdx, mctx->rdi, mctx->rsi,
            mctx->rbp, mctx->rsp, mctx->r8,
            mctx->r9, mctx->r10, mctx->r11,
            mctx->r12, mctx->r13, mctx->r14,
            mctx->r15, mctx->rip, rdcr0(),
            rdcr2(), rdcr3(), rdcr4()
        );
    }
    else {
        printk(
            "\n\e[025453;014mTRAP:%d\e[0m CPU%d TID:%d: ERR:%X rflags=%8X cs=%X ds=%X fs=%X ss=%X\n"
            "\e[025453;015mrax\e[0m=\e[025453;012m%16p\e[0m rbx=\e[025453;12m%16p\e[0m rcx=\e[025453;12m%16p\n\e[0m"
            "\e[025453;015mrdx\e[0m=%16p \e[025453;015mrdi\e[0m=%16p \e[025453;015mrsi\e[0m=%16p\n"
            "\e[025453;015mrbp\e[0m=\e[025453;012m%16p\e[0m rsp=\e[025453;12m%16p\e[0m r8 =\e[025453;12m%16p\n\e[0m"
            "\e[025453;015mr9\e[0m =%16p \e[025453;015mr10\e[0m=%16p \e[025453;015mr11\e[0m=%16p\n"
            "\e[025453;015mr12\e[0m=\e[025453;012m%16p\e[0m r13=\e[025453;12m%16p\e[0m r14=\e[025453;12m%16p\n\e[0m"
            "\e[025453;015mr15\e[0m=%16p \e[025453;015mrip\e[0m=%16p \e[025453;015mcr0\e[0m=%16p\n"
            "\e[025453;015mcr2\e[0m=\e[025453;012m%16p\e[0m cr3=\e[025453;12m%16p\e[0m cr4=\e[025453;12m%16p\n\e[0m",
            mctx->trapno, getcpuid(), thread_self(), mctx->errno, mctx->rflags, mctx->cs, mctx->ds, mctx->fs, mctx->ss,
            mctx->rax, mctx->rbx, mctx->rcx,
            mctx->rdx, mctx->rdi, mctx->rsi,
            mctx->rbp, mctx->rsp, mctx->r8,
            mctx->r9, mctx->r10, mctx->r11,
            mctx->r12, mctx->r13, mctx->r14,
            mctx->r15, mctx->rip, rdcr0(),
            rdcr2(), rdcr3(), rdcr4()
        );
    }
}

void trap(ucontext_t *rctx) {
    time_t          time    = 0;
    arch_thread_t   *arch   = NULL;
    ucontext_t      *uctx   = rctx->uc_link;
    mcontext_t      *mctx   = &uctx->uc_mcontext;

    if (current) {
        arch                = &current->t_arch;
        arch->t_rsvdspace   = rctx;
        uctx->uc_stack      = current_isuser() ?
            arch->t_kstack : arch->t_ustack;
        pushcli();
        uctx->uc_link       = arch->t_ucontext;
        arch->t_ucontext    = uctx;
        popcli();
        uctx->uc_flags      = 0;
        sigemptyset(&uctx->uc_sigmask);
    }

    switch (mctx->trapno) {
    case T_LEG_SYSCALL:
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

    sched_remove_zombies();

    if (!current)
        return;

    if (current_iskilled())
        thread_exit(-EINTR);

    current_lock();
    if (current_testflags(THREAD_STOP))
        thread_stop(current, sched_stopq);
    current_unlock();

    pushcli();
    dispatch_signal();
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

    arch->t_ucontext = uctx->uc_link;
}