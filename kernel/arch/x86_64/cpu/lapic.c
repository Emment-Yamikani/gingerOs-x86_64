#include <arch/lapic.h>
#include <arch/x86_64/msr.h>
#include <sys/system.h>
#include <lib/printk.h>
#include <arch/x86_64/pml4.h>
#include <bits/errno.h>
#include <arch/traps.h>
#include <sys/thread.h>

volatile uint32_t *LAPIC_BASE = 0;

#define ID LAPIC_BASE[0x20 / 4]  // ID register.
#define VER LAPIC_BASE[0x30 / 4] // Version register.
#define TPR LAPIC_BASE[0x80 / 4] // Task priority register.
#define APR LAPIC_BASE[0x90 / 4] // Artribution priority register.
#define PPR LAPIC_BASE[0xA0 / 4] // Processor priority register.

#define EOI LAPIC_BASE[0xB0 / 4]           // End of Interrupt.
#define RRR LAPIC_BASE[0xC0 / 4]           // Remote read register.
#define RDR LAPIC_BASE[0xD0 / 4]           // Logical destination register.
#define DFR LAPIC_BASE[0xE0 / 4]           // Destination format register.
#define SIVR LAPIC_BASE[0xF0 / 4]          // Spurious interrupt vector register.
#define ISR0 LAPIC_BASE[0x100 / 4]         // In service register 0
#define ISR7 LAPIC_BASE[(ISR0 + 0x70) / 4] // ISR 7.
#define TMR0 LAPIC_BASE[0x180 / 4]         // Trigger mode register.
#define TMR7 LAPIC_BASE[(TMR0 + 0x70) / 4] // TMR 7.
#define IRR0 LAPIC_BASE[0x200 / 4]         // Interrupt request register.
#define IRR7 LAPIC_BASE[(IRR0 + 0x70) / 4] // IRR 7.
#define ESR LAPIC_BASE[0x280 / 4]          // Error status register.
#define ICR0 LAPIC_BASE[0x300 / 4]         // Interrupt command register.
#define ICR1 LAPIC_BASE[0x310 / 4]         // Interrupt command register.

#define LVT_TIMER LAPIC_BASE[0x320 / 4] // Timer register.
#define LVT_TSR LAPIC_BASE[0x330 / 4]   // Thermal Sensor register.
#define LVT_PMCR LAPIC_BASE[0x340 / 4]  // Performance monitorig count register.
#define LVT_LINT0 LAPIC_BASE[0x350 / 4] // Local interrupt 0.
#define LVT_LINT1 LAPIC_BASE[0x360 / 4] // Local interrupt 1.
#define LVT_ERR LAPIC_BASE[0x370 / 4]   // Error register.
#define ICR LAPIC_BASE[0x380 / 4]       // Initial count register.
#define CCR LAPIC_BASE[0x390 / 4]       // Current count register.
#define DCR LAPIC_BASE[0x3E0 / 4]       // Divide configurations register.

#define ONESHOT SHL(0b0, 17)
#define PERIODIC SHL(0b1, 17)

#define MASKED BS(16)
#define LEVEL BS(15)
#define LOGICAL_DEST BS(11)

#define SELF 0x40000
#define BCAST 0x80000
#define BCAST_XSELF 0xC0000

#define ASSERT BS(14)

#define FIXED 0x000
#define SMI 0x200
#define NMI 0x400
#define INIT 0x500
#define SIPI 0x600

#define DELIVS BS(12)

#define ENABLED BS(8)

#define X1      0b1011
#define X2      0b0000
#define X4      0b0001
#define X8      0b0010
#define X16     0b0011
#define X32     0b1000
#define X64     0b1001
#define X128    0b1010

void lapic_eoi(void) { EOI = 0; }

void lapic_disable(void) { SIVR = 0xFF; }

int lapic_id(void) { return (ID >> 24) & 0xFF; }

void lapic_enable(void) { SIVR = ENABLED | LAPIC_SPURIOUS; }

int lapic_init(void)
{
    SIVR = ENABLED | LAPIC_SPURIOUS;

    LVT_TIMER = MASKED;
    DCR = X1;
    ICR = 24000;
    LVT_TIMER = PERIODIC | LAPIC_TIMER;

    LVT_LINT0 = MASKED;
    LVT_LINT1 = MASKED;
    LVT_ERR = LAPIC_ERROR;

    ESR = 0;
    ESR = 0;
    lapic_eoi();

    ICR1 = 0;
    ICR0 = INIT | LEVEL | BCAST;
    while (ICR0 & DELIVS);

    TPR = 0;
    return 0;
}

static void microdelay(int us)
{
    while (us--)
        for (int i = 0; i < 1000; i++);
}

void lapic_recalibrate(int hz) {
    (void)hz;
}

void lapic_startup(int id, uint16_t addr)
{
    ICR1 = (id << 24);
    ICR0 = INIT | ASSERT;
    while (ICR0 & DELIVS)
        ;

    ICR1 = (id << 24);
    ICR0 = ICR0 = INIT | LEVEL;
    while (ICR0 & DELIVS)
        ;

    microdelay(10);

    for (int i = 0; i < 2; ++i)
    {
        ICR1 = (id << 24);
        ICR0 = SIPI | (addr >> 12);
        while (ICR0 & DELIVS)
            ;
        microdelay(200);
    }
}

void lapic_timerintr(void)
{
    atomic_inc(&cpu->timer_ticks);
    if (current) {
        atomic_inc(&current->t_sched_attr.age);
        atomic_dec(&current->t_sched_attr.timeslice);
    }
}

void lapic_ipi(int id, int ipi) {
    if (!LAPIC_BASE)
        return;
    ICR1 = (id << 24);
    if (id == -1) // send to self.
        ICR0 = ASSERT | LEVEL | SELF | (ipi & 0xff);
    else if (id == -2) // broadcast to all.
        ICR0 = ASSERT | LEVEL | BCAST | (ipi & 0xff);
    else if (id == -3) // broadcast to all except self.
        ICR0 = ASSERT | LEVEL | BCAST_XSELF | (ipi & 0xff);
    else // send to specific lapic.
        ICR0 = ASSERT | LEVEL | (ipi & 0xff);
    while (ICR0 & DELIVS);
}