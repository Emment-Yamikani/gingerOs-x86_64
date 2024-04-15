#include <arch/cpu.h>
#include <arch/traps.h>
#include <arch/x86_64/isr.h>
#include <arch/x86_64/mmu.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <sys/system.h>

static idt_t idt = {0};

void tss_set(uintptr_t kstack, uint16_t desc __unused) {
    cpu->gdt.tss    = TSS(((uintptr_t)&cpu->tss), (sizeof(cpu->tss) - 1), TSS_SEG, 0x8E);
    memset(&cpu->tss, 0, sizeof cpu->tss);
    cpu->tss.rsp0   = kstack;
}

void gdt_init(void) {
    cpu_t *c = cpu;

    if (!cpu) {
        panic("No CPU struct pointer specified\n");
    }

    memset(&cpu->gdt, 0, sizeof cpu->gdt);

    cpu->gdt.null       = SEG(0, 0, 0, 0);
    cpu->gdt.kcode64    = SEG(0, -1, SEG_CODE, KCODE_SEG);
    cpu->gdt.kdata64    = SEG(0, -1, SEG_DATA, KDATA_SEG);
    cpu->gdt.ucode64    = SEG(0, -1, SEG_CODE, UCODE_SEG64);
    cpu->gdt.udata64    = SEG(0, -1, SEG_DATA, UDATA_SEG);
    cpu->gdt.tss        = TSS(((uintptr_t)&cpu->tss), (sizeof (cpu->tss) - 1), TSS_SEG, 0x8E);

    descptr_t ptr = (descptr_t) {
        .base   = (uintptr_t)&cpu->gdt,
        .limit  = sizeof cpu->gdt - 1,
    };

    ptr.base    = (uintptr_t)&cpu->gdt;
    ptr.limit   = sizeof(gdt_t) - 1;
    asm volatile("lgdt (%%rax)" :: "a"(&ptr) : "memory");
    asm volatile("ltr %%ax"     :: "a"(SEG_TSS64 << 3));
    asm volatile("swapgs;\
        mov $0x10, %%ax;\
        mov %%ax, %%ds;\
        mov %%ax, %%ss;\
        mov $0x0, %%ax;\
        mov %%ax, %%gs;\
        mov %%ax, %%fs;\
        " ::: "ax");
    setcls(c);
}

void setgate(int gate, int istrap, void (*base)() , uint16_t sel, uint8_t dpl, uint8_t ist) {
    idt.entry[gate] = TRAP_GATE(
        istrap, (uintptr_t)base,
        sel, dpl, ist);
}

void idt_init(void) {
    descptr_t ptr = (descptr_t){
        .base   = (uintptr_t)&idt,
        .limit  = sizeof idt - 1,
    };
    loadidt(&ptr);
}

void tvinit(void) {
    setgate(0, 0, isr0, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(1, 0, isr1, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(2, 0, isr2, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(3, 0, isr3, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(4, 0, isr4, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(5, 0, isr5, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(6, 0, isr6, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(7, 0, isr7, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(8, 0, isr8, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(9, 0, isr9, (SEG_KCODE64 << 3), DPL_KRN, 0);

    setgate(10, 0, isr10, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(11, 0, isr11, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(12, 0, isr12, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(13, 0, isr13, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(14, 0, isr14, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(15, 0, isr15, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(16, 0, isr16, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(17, 0, isr17, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(18, 0, isr18, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(19, 0, isr19, (SEG_KCODE64 << 3), DPL_KRN, 0);

    setgate(20, 0, isr20, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(21, 0, isr21, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(22, 0, isr22, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(23, 0, isr23, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(24, 0, isr24, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(25, 0, isr25, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(26, 0, isr26, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(27, 0, isr27, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(28, 0, isr28, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(29, 0, isr29, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(30, 0, isr30, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(31, 0, isr31, (SEG_KCODE64 << 3), DPL_KRN, 0);

    setgate(IRQ(0), 0, irq0, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(1), 0, irq1, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(2), 0, irq2, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(3), 0, irq3, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(4), 0, irq4, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(5), 0, irq5, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(6), 0, irq6, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(7), 0, irq7, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(8), 0, irq8, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(9), 0, irq9, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(10), 0, irq10, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(11), 0, irq11, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(12), 0, irq12, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(13), 0, irq13, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(14), 0, irq14, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(15), 0, irq15, (SEG_KCODE64 << 3), DPL_KRN, 0);

    setgate(IRQ(16), 0, irq16, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(17), 0, irq17, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(18), 0, irq18, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(19), 0, irq19, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(20), 0, irq20, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(21), 0, irq21, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(22), 0, irq22, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(23), 0, irq23, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(24), 0, irq24, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(25), 0, irq25, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(26), 0, irq26, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(27), 0, irq27, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(28), 0, irq28, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(29), 0, irq29, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(30), 0, irq30, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(31), 0, irq31, (SEG_KCODE64 << 3), DPL_KRN, 0);

    setgate(T_LEG_SYSCALL, 1, isr128, (SEG_KCODE64 << 3), DPL_USR, 0);
}