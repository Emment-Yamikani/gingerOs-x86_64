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

void dump_tf(tf_t *tf, int halt) {
    if (halt)
        panic("\n\e[025453;014mTRAP:%d\e[0m CPU%d TID:%d: ERR:%X rflags=%8X cs=%X ds=%X fs=%X ss=%X\n"
              "\e[025453;015mrax\e[0m=\e[025453;016m%16p\e[0m \e[025453;015mrbx\e[0m=\e[025453;12m%16p\e[0m \e[025453;015mrcx\e[0m=\e[025453;12m%16p\n\e[0m"
              "\e[025453;015mrdx\e[0m=%16p \e[025453;015mrdi\e[0m=%16p \e[025453;015mrsi\e[0m=%16p\n"
              "\e[025453;03mrbp\e[0m=\e[025453;03m%16p\e[0m \e[025453;03mrsp\e[0m=\e[025453;03m%16p\e[0m \e[025453;015mr8\e[0m =\e[025453;12m%16p\n\e[0m"
              "\e[025453;015mr9\e[0m =%16p \e[025453;015mr10\e[0m=%16p \e[025453;015mr11\e[0m=%16p\n"
              "\e[025453;015mr12\e[0m=\e[025453;012m%16p\e[0m \e[025453;015mr13\e[0m=\e[025453;12m%16p\e[0m \e[025453;015mr14\e[0m=\e[025453;12m%16p\n\e[0m"
              "\e[025453;015mr15\e[0m=%16p \e[025453;015mrip\e[0m=\e[025453;016m%16p\e[0m \e[025453;015mcr0\e[0m=%16p\n"
              "\e[025453;015mcr2\e[0m=\e[025453;016m%16p\e[0m \e[025453;015mcr3\e[0m=\e[025453;12m%16p\e[0m \e[025453;015mcr4\e[0m=\e[025453;12m%16p\n\e[0m",
              tf->trapno, getcpuid(), thread_self(), tf->err_code, tf->rflags, tf->cs, tf->ds, tf->fs, tf->ss, 
              tf->rax, tf->rbx, tf->rcx,
              tf->rdx, tf->rdi, tf->rsi,
              tf->rbp, tf->rsp, tf->r8,
              tf->r9, tf->r10, tf->r11,
              tf->r12, tf->r13, tf->r14,
              tf->r15, tf->rip, rdcr0(),
              rdcr2(), rdcr3(), rdcr4());
    else
        printk("\n\e[025453;014mTRAP:%d\e[0m CPU%d TID:%d: ERR:%X rflags=%8X cs=%X ds=%X fs=%X ss=%X\n"
               "\e[025453;015mrax\e[0m=\e[025453;012m%16p\e[0m rbx=\e[025453;12m%16p\e[0m rcx=\e[025453;12m%16p\n\e[0m"
               "\e[025453;015mrdx\e[0m=%16p \e[025453;015mrdi\e[0m=%16p \e[025453;015mrsi\e[0m=%16p\n"
               "\e[025453;015mrbp\e[0m=\e[025453;012m%16p\e[0m rsp=\e[025453;12m%16p\e[0m r8 =\e[025453;12m%16p\n\e[0m"
               "\e[025453;015mr9\e[0m =%16p \e[025453;015mr10\e[0m=%16p \e[025453;015mr11\e[0m=%16p\n"
               "\e[025453;015mr12\e[0m=\e[025453;012m%16p\e[0m r13=\e[025453;12m%16p\e[0m r14=\e[025453;12m%16p\n\e[0m"
               "\e[025453;015mr15\e[0m=%16p \e[025453;015mrip\e[0m=%16p \e[025453;015mcr0\e[0m=%16p\n"
               "\e[025453;015mcr2\e[0m=\e[025453;012m%16p\e[0m cr3=\e[025453;12m%16p\e[0m cr4=\e[025453;12m%16p\n\e[0m",
               tf->trapno, getcpuid(), thread_self(), tf->err_code, tf->rflags, tf->cs, tf->ds, tf->fs, tf->ss,
               tf->rax, tf->rbx, tf->rcx,
               tf->rdx, tf->rdi, tf->rsi,
               tf->rbp, tf->rsp, tf->r8,
               tf->r9, tf->r10, tf->r11,
               tf->r12, tf->r13, tf->r14,
               tf->r15, tf->rip, rdcr0(),
               rdcr2(), rdcr3(), rdcr4());
}

void trap(tf_t *tf) {
    time_t time = 0;
    switch (tf->trapno) {
    case T_LEG_SYSCALL:
        current->t_arch.t_tf = tf;
        do_syscall(tf);
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
        arch_do_page_fault(tf);
        lapic_eoi();
        break;
    default:
        dump_tf(tf, 1);
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

    signal_handle(tf);

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