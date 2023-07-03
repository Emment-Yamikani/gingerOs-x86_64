#pragma once

#include <lib/stdint.h>

extern int lapic_id(void);
extern int lapic_init(void);
extern void lapic_eoi(void);
extern void lapic_timerintr(void);
extern void lapic_ipi(int id, int ipi);
extern void lapic_startup(int id, uint16_t addr);