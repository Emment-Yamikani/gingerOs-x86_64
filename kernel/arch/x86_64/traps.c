#include <arch/x86_64/context.h>
#include <lib/printk.h>
#include <sys/system.h>
#include <lib/stddef.h>
#include <arch/x86_64/system.h>
#include <arch/traps.h>
#include <arch/lapic.h>
#include <arch/cpu.h>
#include <sys/thread.h>

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

extern void hpet_intr(void);

void trap(tf_t *tf) {

    if (current) {
        if (thread_killed(current))
            thread_exit(-EINTR);
    }

    switch (tf->trapno)
    {
    case IRQ(HPET):
        hpet_intr();
        lapic_eoi();
        break;
    case T_FPU_NM:
        coprocessor_except();
        break;
    case T_SIMD_XM:
        simd_fp_except();
        break;
    case T_PGFAULT:
        panic("[CPU%d] PF: errno: %x, cr2: %lX, rip: %p\n", cpu_id, tf->errno, rdcr2(), tf->rip);
        break;
    case LAPIC_ERROR:
        lapic_eoi();
        break;
    case LAPIC_SPURIOUS:
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
        panic("[CPU%d] trap(%d): errno: %x, rbp: %p, cr2: %lX, rip: %p\n", cpu_id, tf->trapno, tf->errno, tf->rbp, rdcr2(), tf->rip);
        break;
    }

    if (!current)
        return;

    if (thread_killed(current))
        thread_exit(-EINTR);

    if (!atomic_read(&current->t_sched_attr.timeslice))
        thread_yield();

    if (thread_killed(current))
        thread_exit(-EINTR);
}