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
#define features0_SYSCALL     BS(16)  // SYSCALL
#define features0_PDCM        BS(15)  // Perf/Debug Capability MSR
#define features0_xTPR        BS(14)  // Update Control
#define features0_CMPXCHG16B  BS(13)  // cmpxchg16B
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

#define features1_PBE   BS(63)    // Pend. Brk. EN.
#define features1_TM    BS(61)    // Therm. Monitor
#define features1_HTT   BS(62)    // Multi-threading
#define features1_SS    BS(59)    // Self Snoop
#define features1_SSE2  BS(58)    // SSE2 Extensions
#define features1_SSE   BS(57)    // SSE Extensions
#define features1_FXSR  BS(56)    // FXSAVE/FXRSTOR
#define features1_MMX   BS(55)    // MMX Technology
#define features1_ACPI  BS(54)    // Thermal Monitor and Clock Ctrl
#define features1_DS    BS(53)    // Debug Store
#define features1_XD    BS(52)    // Execute disabled
#define features1_CLFSH BS(51)    // CLFLUSH instruction
#define features1_PSN   BS(50)    // Processor Serial Number
#define features1_PSE36 BS(49)    //  Page Size Extension
#define features1_PAT   BS(48)    // Page Attribute Table
#define features1_CMOV  BS(47)    // Conditional Move/Compare Instruction
#define features1_MCA   BS(46)    // Machine Check Architecture
#define features1_PGE   BS(45)    // PTE Global Bit
#define features1_MTRR  BS(44)    // Memory Type Range Registers
#define features1_SEP   BS(43)    // SYSENTER and SYSEXIT
#define features1_LM    BS(42)      // Long mode.
#define features1_APIC  BS(41)     // APIC on Chip
#define features1_CX8   BS(40)     // CMPXCHG8B Inst.
#define features1_MCE   BS(39)     // Machine Check Exception
#define features1_PAE   BS(38)     // Physical Address Extensions
#define features1_MSR   BS(37)     // RDMSR and WRMSR Support
#define features1_TSC   BS(36)     // Time Stamp Counter
#define features1_PSE   BS(35)     // Page Size Extensions
#define features1_DE    BS(34)     // Debugging Extensions
#define features1_VME   BS(33)     // Virtual-8086 Mode Enhancement
#define features1_FPU   BS(32)     // x87 FPU on Chip

typedef struct cpu {
    uint64_t ncli;
    uint64_t intena;
    uint64_t version;

    uint64_t apicID;
    uint64_t freq;
    uint64_t flags;
    uint64_t features;
    uint64_t timer_ticks;
    
    tss_t tss;
    gdt_t gdt;
    
    context_t *ctx;

    thread_t *thread;
    sched_queue_t *queueq;

    union {
        struct {
            uint16_t pas: 8;
            uint16_t las: 8;
        };
        uint64_t raw;
    }addr_size;
    char vendor[16];
    char brand_string[64];
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
