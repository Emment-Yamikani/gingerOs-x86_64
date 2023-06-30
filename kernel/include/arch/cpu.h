#pragma once

#include <lib/stdint.h>
#include <arch/x86_64/mmu.h>
#include <arch/x86_64/cpu.h>
#include <lib/types.h>
#include <sys/system.h>
#include <lib/stddef.h>
#include <sys/sched.h>
#include <arch/x86_64/context.h>

#define features0_RDRAND      BS(30)
#define features0_F16C        BS(29)
#define features0_AVX         BS(28)
#define features0_OSXSAVE     BS(27)
#define features0_XSAVE       BS(26)
#define features0_AES         BS(25)
#define features0_TSC         BS(24)  // Deadline
#define features0_POPCNT      BS(23)
#define features0_MOVBE       BS(22)
#define features0_x2APIC      BS(21)
#define features0_SSE4_2      BS(20)  // SSE4.2
#define features0_SSE4_1      BS(19)  // SSE4.1
#define features0_DCA         BS(18)  // Direct Cache Access
#define features0_PCID        BS(17)  // Process-context Identifiers
#define features0_PDCM        BS(15)  // Perf/Debug Capability MSR
#define features0_xTPR        BS(14)  // Update Control
#define features0_CMPXCHG16B  BS(13)
#define features0_FMA         BS(12)  // Fused Multiply Add
#define features0_SDBG        BS(11)
#define features0_CNXT        BS(10)  // ID — L1 Context ID
#define features0_SSSE3       BS(9)   // SSSE3 Extensions
#define features0_TM2         BS(8)   // Thermal Monitor 2
#define features0_EIST        BS(7)   // Enhanced Intel SpeedStep® Technology
#define features0_SMX         BS(6)   // Safer Mode Extensions
#define features0_VMX         BS(5)   // Virtual Machine Extensions
#define features0_DS          BS(4)   // CPL — CPL Qualified Debug Store
#define features0_MONITOR     BS(3)   // MONITOR/MWAIT
#define features0_DTES64      BS(2)   // 64-bit DS Area
#define features0_PCLMULQDQ   BS(1)   // Carryless Multiplication
#define features0_SSE3        BS(0)   // SSE3 Extensions

#define features1_PBE   BS(31)    // Pend. Brk. EN.
#define features1_TM    BS(29)    // Therm. Monitor
#define features1_HTT   BS(28)    // Multi-threading
#define features1_SS    BS(27)    // Self Snoop
#define features1_SSE2  BS(26)    // SSE2 Extensions
#define features1_SSE   BS(25)    // SSE Extensions
#define features1_FXSR  BS(24)    // FXSAVE/FXRSTOR
#define features1_MMX   BS(23)    // MMX Technology
#define features1_ACPI  BS(22)    // Thermal Monitor and Clock Ctrl
#define features1_DS    BS(21)    // Debug Store
#define features1_CLFSH BS(19)    // CLFLUSH instruction
#define features1_PSN   BS(18)    // Processor Serial Number
#define features1_PSE36 BS(17)    //  Page Size Extension
#define features1_PAT   BS(16)    // Page Attribute Table
#define features1_CMOV  BS(15)    // Conditional Move/Compare Instruction
#define features1_MCA   BS(14)    // Machine Check Architecture
#define features1_PGE   BS(13)    // PTE Global Bit
#define features1_MTRR  BS(12)    // Memory Type Range Registers
#define features1_SEP   BS(11)    // SYSENTER and SYSEXIT
#define features1_APIC  BS(9)     // APIC on Chip
#define features1_CX8   BS(8)     // CMPXCHG8B Inst.
#define features1_MCE   BS(7)     // Machine Check Exception
#define features1_PAE   BS(6)     // Physical Address Extensions
#define features1_MSR   BS(5)     // RDMSR and WRMSR Support
#define features1_TSC   BS(4)     // Time Stamp Counter
#define features1_PSE   BS(3)     // Page Size Extensions
#define features1_DE    BS(2)     // Debugging Extensions
#define features1_VME   BS(1)     // Virtual-8086 Mode Enhancement
#define features1_FPU   BS(0)     // x87 FPU on Chip

typedef struct cpu {
    int ncli;
    int intena;
    uint8_t apic_id;
    char vendor[16];
    char brand_string[64];
    size_t freq;
    atomic_t timer_ticks;
    uint32_t syscall : 1;
    uint32_t execute_disable : 1;
    uint32_t long_mode : 1;
    uint64_t flags;
    uint32_t version;
    union {
        struct {
            uint16_t pas: 8;
            uint16_t las: 8;
        };
        uint32_t raw;
    }addr_size;

    
    uint32_t features0;
    uint32_t features1;
    gdt_t gdt;
    tss_t tss;
    
    context_t *ctx;

    thread_t *thread;
    sched_queue_t *queueq;
} cpu_t;

#define MAXNCPU 64 // maximum supported cpus

#define CPU_ENABLED     BS(0)   // cpu is usable(enabled).
#define CPU_ISBSP       BS(1)   // cpu is a bootstrap processor. 
#define CPU_ONLINE      BS(2)   // cpu is online.
#define CPU_PANICED     BS(32)  // cpu has paniced


extern cpu_t *cpus[MAXNCPU];

#define cpu         (get_cpu_locale())
#define cpu_id      (cpu_locale_id())
#define current     (cpu->thread)
#define cpu_flags   (cpu->flags)
#define isbsp()     (cpu_flags & CPU_ISBSP)
#define ready_queue (cpu->queueq)

extern void cpu_init(cpu_t *);
extern cpu_t *get_cpu_locale(void);
extern void set_cpu_locale(cpu_t *);
extern uintptr_t readgs_base();
extern void loadgs_base(uintptr_t base);
extern int cpu_locale_id(void);
extern int get_cpu_count(void);
extern int enumerate_cpus(void);
extern int bootothers(void);
extern void cpu_get_features(void);
