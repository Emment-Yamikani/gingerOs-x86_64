#pragma once

#define IRQ_OFFSET  32
#define IRQ(i)  ((i) + IRQ_OFFSET)


#define PS2_KBD         1
#define T_FPU_NM        7
#define T_SIMD_XM       19
#define T_PGFAULT       14
#define TLBSHOOTDWN     17

#define LEG_PIT         2
#define HPET            2
#define IRQ_RTC         8
#define LAPIC_ERROR     IRQ(18)
#define LAPIC_SPURIOUS  IRQ(19)
#define LAPIC_TIMER     IRQ(20)
#define LAPIC_IPI       IRQ(31)

#define T_LEG_SYSCALL   128
