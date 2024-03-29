#include <arch/cpu.h>
#include <arch/lapic.h>
#include <arch/x86_64/ipi.h>
#include <bits/errno.h>
#include <ds/queue.h>
#include <ginger/jiffies.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <arch/traps.h>

void tlb_shootdown_handler(void) {
    wrcr3(rdcr3());
}

int tlb_shootdown(uintptr_t pml4, uintptr_t viraddr) {
    (void)pml4;
    (void)viraddr;
    lapic_send_ipi(TLB_SHTDWN, IPI_ALLXSELF);
    return 0;
}

void send_tlb_shootdown(uintptr_t pml4, uintptr_t viraddr) {
    invlpg(viraddr);
    tlb_shootdown(pml4, viraddr);
}

int i64_send_ipi(int dst, int ipi, void *arg0, void *arg1, void *arg2);