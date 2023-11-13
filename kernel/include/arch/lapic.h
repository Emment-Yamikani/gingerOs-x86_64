#pragma once

#include <lib/stdint.h>

extern int lapic_id(void);
extern int lapic_init(void);
extern void lapic_eoi(void);
extern void lapic_timerintr(void);
// extern void lapic_setaddr(uintptr_t);
void lapic_send_ipi(int ipi, int dst);
void lapic_recalibrate(long hz);
extern void lapic_startup(int id, uint16_t addr);