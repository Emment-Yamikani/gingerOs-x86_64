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

void dump_tf(tf_t *tf, int halt) {
    if (!halt)
    panic("r15: %p\nr14: %p\n"
           "r13: %p\nr12: %p\nr11: %p\nr10: %p\nr9: %p\nr8: %p\n"
           "rbp: %p\nrsi: %p\nrdi: %p\nrdx: %p\nrcx: %p\nrbx: %p\n"
           "fs: %p\ntrapno: %p\nerrno: %p\nrip: %p\ncs: %p\n"
           "rflags: %p\nrsp: %p\nss: %p\n",
           tf->r15, tf->r14, tf->r13, tf->r12,
           tf->r11, tf->r10, tf->r9, tf->r8, tf->rbp,
           tf->rsi, tf->rdi, tf->rdx, tf->rcx, tf->rbx, /*tf->gs,*/
           tf->fs, tf->trapno, tf->errno, tf->rip, tf->cs,
           tf->rflags, tf->rsp, tf->ss);
    else
    printk("r15: %p\nr14: %p\n"
           "r13: %p\nr12: %p\nr11: %p\nr10: %p\nr9: %p\nr8: %p\n"
           "rbp: %p\nrsi: %p\nrdi: %p\nrdx: %p\nrcx: %p\nrbx: %p\n"
           "fs: %p\ntrapno: %p\nerrno: %p\nrip: %p\ncs: %p\n"
           "rflags: %p\nrsp: %p\nss: %p\n",
           tf->r15, tf->r14, tf->r13, tf->r12,
           tf->r11, tf->r10, tf->r9, tf->r8, tf->rbp,
           tf->rsi, tf->rdi, tf->rdx, tf->rcx, tf->rbx, /*tf->gs,*/
           tf->fs, tf->trapno, tf->errno, tf->rip, tf->cs,
           tf->rflags, tf->rsp, tf->ss);
}

void trap(tf_t *tf) {
    time_t time = 0;
    switch (tf->trapno) {
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
    case T_PGFAULT:
        panic("[CPU%d] PF: thread[%d]: errno: %x, cr2: %p, rip: %p\n",
            cpu_id, thread_self(), tf->errno, rdcr2(), tf->rip);
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
    case LAPIC_IPI:
        printk("cpu%d: ipi test\n", cpu_id);
        lapic_eoi();
        break;
    default:
        panic("[CPU%d] thread[%d]: trap(%d): errno: %x, rbp: %p, cr2: %p, rip: %p\n",
            cpu_id, thread_self(), tf->trapno, tf->errno, tf->rbp, rdcr2(), tf->rip);
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
    time = current->t_sched_attr.timeslice;
    if (current_testflags(THREAD_STOP))
        thread_stop(current, sched_stopq);
    current_unlock();

    if (time <= 0)
        thread_yield();

    if (current_iskilled())
        thread_exit(-EINTR);
}