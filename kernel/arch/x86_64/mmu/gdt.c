#include <arch/x86_64/mmu.h>
#include <lib/printk.h>
#include <arch/cpu.h>
#include <arch/x86_64/isr.h>
#include <sys/system.h>
#include <arch/traps.h>
#include <lib/string.h>

static idt_t idt = {0};

void gdt_init(cpu_t *c) {
    if (!c)
        panic("No CPU struct pointer specified\n");
    memset(&c->gdt, 0, sizeof c->gdt);
    c->gdt.null = SEG(0, 0, 0, 0);
    c->gdt.kcode64 = SEG(0, -1, SEG_CODE, KCODE_SEG);
    c->gdt.kdata64 = SEG(0, -1, SEG_DATA, KDATA_SEG);
    c->gdt.ucode64 = SEG(0, -1, SEG_CODE, UCODE_SEG64);
    c->gdt.udata64 = SEG(0, -1, SEG_DATA, UDATA_SEG);
    c->gdt.kcpu = SEG(0, (sizeof(cpu_t) - 1), SEG_DATA, KDATA_SEG);
    c->gdt.tss = TSS(((uintptr_t)&c->tss), (sizeof (c->tss) - 1), TSS_SEG, 0x8E);

    descptr_t ptr = (descptr_t) {
        .base = (uintptr_t)&c->gdt,
        .limit = sizeof c->gdt - 1,
    };

    loadgdt64(&ptr, (SEG_KCODE64 << 3), (SEG_KCPU64 << 3), (SEG_KDATA64 << 3));
    set_cpu_locale(c);
    loadtr(SEG_TSS64 << 3);
}

void setgate(int gate, int istrap, void (*base)() , uint16_t sel, uint8_t dpl, uint8_t ist) {
    idt.entry[gate] = TRAP_GATE(istrap, (uintptr_t)base, sel, dpl, ist);
}

void idt_init(void) {
    descptr_t ptr = (descptr_t){
        .base = (uintptr_t)&idt,
        .limit = sizeof idt - 1,
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
    setgate(IRQ(32), 0, irq32, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(33), 0, irq33, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(34), 0, irq34, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(35), 0, irq35, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(36), 0, irq36, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(37), 0, irq37, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(38), 0, irq38, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(39), 0, irq39, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(40), 0, irq40, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(41), 0, irq41, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(42), 0, irq42, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(43), 0, irq43, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(44), 0, irq44, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(45), 0, irq45, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(46), 0, irq46, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(47), 0, irq47, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(48), 0, irq48, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(49), 0, irq49, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(50), 0, irq50, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(51), 0, irq51, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(52), 0, irq52, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(53), 0, irq53, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(54), 0, irq54, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(55), 0, irq55, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(56), 0, irq56, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(57), 0, irq57, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(58), 0, irq58, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(59), 0, irq59, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(60), 0, irq60, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(61), 0, irq61, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(62), 0, irq62, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(63), 0, irq63, (SEG_KCODE64 << 3), DPL_KRN, 0);
    setgate(IRQ(64), 0, irq64, (SEG_KCODE64 << 3), DPL_KRN, 0);

    setgate(T_LEG_SYSCALL, 1, isr128, (SEG_KCODE64 << 3), DPL_USR, 0);
}