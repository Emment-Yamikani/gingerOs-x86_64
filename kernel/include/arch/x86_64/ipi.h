#pragma once

#define NIPI 8

#define IPI_ALL         (-2) // broadcast to all.
#define IPI_SELF        (-1) // send to self.
#define IPI_ALLXSELF    (-3) // broadcast to all except self.

#define IPI_SYNC        0
#define IPI_TLBSHTDWN   1

typedef struct ipi_t {
    int src;
    void *arg0;
    void *arg1;
    void *arg2;
} ipi_t;

typedef struct tlb_entry_t {
    uintptr_t pml4;
    long      count;
    uintptr_t viraddr;
} tlb_entry_t;

void tlb_shootdown_handler(void);
int tlb_shootdown(uintptr_t pml4, uintptr_t viraddr);
void send_tlb_shootdown(uintptr_t pml4, uintptr_t viraddr);
int i64_send_ipi(int dst, int ipi, void *arg0, void *arg1, void *arg2);