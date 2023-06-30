#pragma once

#include <lib/stdint.h>

#define IA32_APIC_BASE      0x1B    // “Local APIC Status and Location,”

#define IA32_X2APIC_APICID  0x802   //x2APIC ID Register (R/O)
#define IA32_X2APIC_VERSION 0x803   //x2APIC Version Register (R/O)
#define IA32_X2APIC_TPR     0x808   //x2APIC Task Priority Register (R/W)
#define IA32_X2APIC_PPR     0x80A   //x2APIC Processor Priority Register (R/O)
#define IA32_X2APIC_EOI     0x80B   //x2APIC EOI Register (W/O)
#define IA32_X2APIC_LDR     0x80D   //x2APIC Logical Destination Register (R/O)
#define IA32_X2APIC_SIVR    0x80F   //x2APIC Spurious Interrupt Vector Register (R/W)
#define IA32_X2APIC_ISR0    0x810   //x2APIC In-Service Register Bits [31:0] (R/O)
#define IA32_X2APIC_ISR1    0x811   //x2APIC In-Service Register Bits [63:32] (R/O)
#define IA32_X2APIC_ISR2    0x812   //x2APIC In-Service Register Bits [95:64] (R/O)
#define IA32_X2APIC_ISR3    0x813   //x2APIC In-Service Register Bits [127:96] (R/O)
#define IA32_X2APIC_ISR4    0x814   //x2APIC In-Service Register Bits [159:128] (R/O)
#define IA32_X2APIC_ISR5    0x815   //x2APIC In-Service Register Bits [191:160] (R/O)
#define IA32_X2APIC_ISR6    0x816   //x2APIC In-Service Register Bits [223:192] (R/O)
#define IA32_X2APIC_ISR7    0x817   //x2APIC In-Service Register Bits [255:224] (R/O)
#define IA32_X2APIC_TMR0    0x818   //x2APIC Trigger Mode Register Bits [31:0] (R/O)
#define IA32_X2APIC_TMR1    0x819   //x2APIC Trigger Mode Register Bits [63:32] (R/O)
#define IA32_X2APIC_TMR2    0x81A   //x2APIC Trigger Mode Register Bits [95:64] (R/O)

#define IA32_X2APIC_TMR3    0x81B   //x2APIC Trigger Mode Register Bits [127:96] (R/O)
#define IA32_X2APIC_TMR4    0x81C   //x2APIC Trigger Mode Register Bits [159:128] (R/O)
#define IA32_X2APIC_TMR5    0x81D   //x2APIC Trigger Mode Register Bits [191:160] (R/O)
#define IA32_X2APIC_TMR6    0x81E   //x2APIC Trigger Mode Register Bits [223:192] (R/O)
#define IA32_X2APIC_TMR7    0x81F   //x2APIC Trigger Mode Register Bits [255:224] (R/O)
#define IA32_X2APIC_IRR0    0x820   //x2APIC Interrupt Request Register Bits [31:0] (R/O)
#define IA32_X2APIC_IRR1    0x821   //x2APIC Interrupt Request Register Bits [63:32] (R/O)
#define IA32_X2APIC_IRR2    0x822   //x2APIC Interrupt Request Register Bits [95:64] (R/O)
#define IA32_X2APIC_IRR3    0x823   //x2APIC Interrupt Request Register Bits [127:96] (R/O)
#define IA32_X2APIC_IRR4    0x824   //x2APIC Interrupt Request Register Bits [159:128] (R/O)
#define IA32_X2APIC_IRR5    0x825   //x2APIC Interrupt Request Register Bits [191:160] (R/O)
#define IA32_X2APIC_IRR6    0x826   //x2APIC Interrupt Request Register Bits [223:192] (R/O)
#define IA32_X2APIC_IRR7    0x827   //x2APIC Interrupt Request Register Bits [255:224] (R/O)
#define IA32_X2APIC_ESR     0x828   //x2APIC Error Status Register (R/W)
#define IA32_X2APIC_LVT_CMCI    0x82F   //x2APIC LVT Corrected Machine Check Interrupt Register(R/W)
#define IA32_X2APIC_ICR         0x830   //x2APIC Interrupt Command Register (R/W)
#define IA32_X2APIC_LVT_TIMER   0x832   //x2APIC LVT Timer Interrupt Register (R/W)
#define IA32_X2APIC_LVT_THERMAL 0x833   //x2APIC LVT Thermal Sensor Interrupt Register (R/W)
#define IA32_X2APIC_LVT_PMI     0x834   //x2APIC LVT Performance Monitor Register (R/W)
#define IA32_X2APIC_LVT_LINT0   0x835   //x2APIC LVT LINT0 Register (R/W)
#define IA32_X2APIC_LVT_LINT1   0x836   //x2APIC LVT LINT1 Register (R/W)
#define IA32_X2APIC_LVT_ERROR   0x837   //x2APIC LVT Error Register (R/W)
#define IA32_X2APIC_INIT_COUNT  0x838   //x2APIC Initial Count Register (R/W)
#define IA32_X2APIC_CUR_COUNT   0x839   //x2APIC Current Count Register (R/O)
#define IA32_X2APIC_DIV_CONF    0x83E   //x2APIC Divide Configuration Register (R/W)
#define IA32_X2APIC_SELF_IPI    0x83F   //x2APIC Self IPI Register (W/O)

#define IA32_EFER               0xC0000080  //Extended Feature Enables
#define IA32_STAR               0xC0000081  //System Call Target Address (R/W)
#define IA32_LSTAR              0xC0000082  //IA-32e Mode System Call Target Address (R/W)
#define IA32_FMASK              0xC0000084  //System Call Flag Mask (R/W)
#define IA32_FS_BASE            0xC0000100  //Map of BASE Address of FS (R/W)
#define IA32_GS_BASE            0xC0000101  //Map of BASE Address of GS (R/W)
#define IA32_KERNEL_GS_BASE     0xC0000102  //Swap Target of BASE Address of GS (R/W)

extern void     wrmsr(uint64_t msr, uint64_t val);
extern uint64_t rdmsr(uint64_t msr);