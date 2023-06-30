#pragma once

#define IRQ_OFFSET  32
#define IRQ(i)  ((i) + IRQ_OFFSET)


#define PS2_KBD         1
#define LEG_PIT         2
#define T_PGFAULT       14
#define HPET            16
#define TLBSHOOTDWN     17

#define LAPIC_ERROR     IRQ(18)
#define LAPIC_SPURIOUS  IRQ(19)
#define LAPIC_TIMER     IRQ(20)

#define T_LEG_SYSCALL   128
