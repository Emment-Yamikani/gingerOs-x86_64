#pragma once

#include <lib/stdint.h>
#include <arch/x86_64/mmu.h>
#include <arch/x86_64/cpu.h>
#include <lib/types.h>
#include <sys/system.h>
#include <lib/stddef.h>
#include <sys/sched.h>
#include <arch/x86_64/context.h>
#include <arch/x86_64/ipi.h>

#define CPU_PBE             BS(63)  // Pend. Brk. EN.
#define CPU_TM              BS(61)  // Therm. Monitor
#define CPU_HTT             BS(62)  // Multi-threading
#define CPU_SS              BS(59)  // Self Snoop
#define CPU_SSE2            BS(58)  // SSE2 Extensions
#define CPU_SSE             BS(57)  // SSE Extensions
#define CPU_FXSR            BS(56)  // FXSAVE/FXRSTOR
#define CPU_MMX             BS(55)  // MMX Technology
#define CPU_ACPI            BS(54)  // Thermal Monitor and Clock Ctrl
#define CPU_DS              BS(53)  // Debug Store
#define CPU_XD              BS(52)  // Execute disabled
#define CPU_CLFSH           BS(51)  // CLFLUSH instruction
#define CPU_PSN             BS(50)  // Processor Serial Number
#define CPU_PSE36           BS(49)  // Page Size Extension
#define CPU_PAT             BS(48)  // Page Attribute Table
#define CPU_CMOV            BS(47)  // Conditional Move/Compare Instruction
#define CPU_MCA             BS(46)  // Machine Check Architecture
#define CPU_PGE             BS(45)  // PTE Global Bit
#define CPU_MTRR            BS(44)  // Memory Type Range Registers
#define CPU_SEP             BS(43)  // SYSENTER and SYSEXIT
#define CPU_LM              BS(42)  // Long mode.
#define CPU_APIC            BS(41)  // APIC on Chip
#define CPU_CX8             BS(40)  // CMPXCHG8B Inst.
#define CPU_MCE             BS(39)  // Machine Check Exception
#define CPU_PAE             BS(38)  // Physical Address Extensions
#define CPU_MSR             BS(37)  // RDMSR and WRMSR Support
#define CPU_TSC             BS(36)  // Time Stamp Counter
#define CPU_PSE             BS(35)  // Page Size Extensions
#define CPU_DE              BS(34)  // Debugging Extensions
#define CPU_VME             BS(33)  // Virtual-8086 Mode Enhancement
#define CPU_FPU             BS(32)  // x87 FPU on Chip

#define CPU_RDRAND          BS(30)
#define CPU_F16C            BS(29)
#define CPU_AVX             BS(28)
#define CPU_OSXSAVE         BS(27)
#define CPU_XSAVE           BS(26)
#define CPU_AES             BS(25)
#define CPU_TSC_DL          BS(24)  // Deadline
#define CPU_POPCNT          BS(23)
#define CPU_MOVBE           BS(22)
#define CPU_x2APIC          BS(21)
#define CPU_SSE4_2          BS(20)  // SSE4.2
#define CPU_SSE4_1          BS(19)  // SSE4.1
#define CPU_DCA             BS(18)  // Direct Cache Access
#define CPU_PCID            BS(17)  // Process-context Identifiers
#define CPU_SYSCALL         BS(16)  // SYSCALL
#define CPU_PDCM            BS(15)  // Perf/Debug Capability MSR
#define CPU_xTPR            BS(14)  // Update Control
#define CPU_CMPXCHG16B      BS(13)  // cmpxchg16B
#define CPU_FMA             BS(12)  // Fused Multiply Add
#define CPU_SDBG            BS(11)
#define CPU_CNXT            BS(10)  // ID — L1 Context ID
#define CPU_SSSE3           BS(9)   // SSSE3 Extensions
#define CPU_TM2             BS(8)   // Thermal Monitor 2
#define CPU_EIST            BS(7)   // Enhanced Intel SpeedStep® Technology
#define CPU_SMX             BS(6)   // Safer Mode Extensions
#define CPU_VMX             BS(5)   // Virtual Machine Extensions
#define CPU_QDS             BS(4)   // CPL — CPL Qualified Debug Store
#define CPU_MONITOR         BS(3)   // MONITOR/MWAIT
#define CPU_DTES64          BS(2)   // 64-bit DS Area
#define CPU_PCLMULQDQ       BS(1)   // Carryless Multiplication
#define CPU_SSE3            BS(0)   // SSE3 Extensions

#define CR0_MP              BS(1)   // Monitor Coprocessor.
#define CR0_EM              BS(2)   // Emulation.
#define CR0_TS              BS(3)   // Task Switched.
#define CR0_ET              BS(4)   // Extension Type.
#define CR0_NE              BS(5)   // Numerical Error.
#define CR0_WP              BS(16)  // Write Protect.
#define CR0_AM              BS(18)  // Alignment Masking.
#define CR0_NW              BS(29)  // Not Write-through.
#define CR0_CD              BS(30)  // Cache Disable.

#define CR4_VME             BS(0)    // Virtual-8086 Mode Extensions.
#define CR4_PVI             BS(1)    // Protected-Mode Virtual Interrupts .
#define CR4_TSD             BS(2)    // Time Stamp Disable.
#define CR4_DE              BS(3)    // Debugging Extensions.
#define CR4_PSE             BS(4)    // Page Size Extensions.
#define CR4_PAE             BS(5)    // Machine-Check Enable.
#define CR4_MCE             BS(6)    // Page Global Enable.
#define CR4_PGE             BS(7)    // Page Global Enable.
#define CR4_PCE             BS(8)    // Performance-Monitoring Counter Enable.
#define CR4_OSFXRS          BS(9)    // Operating System Support for FXSAVE and FXRSTOR instructions.
#define CR4_OSXMMEXCPT      BS(10)   // Operating System Support for Unmasked SIMD Floating-Point Exceptions.
#define CR4_UMIP            BS(11)   // User-Mode Instruction Prevention.
#define CR4_VMXE            BS(13)   // VMX-Enable Bit.
#define CR4_SMXE            BS(14)   // SMX-Enable Bit.
#define CR4_FSGSBASE        BS(16)   // FSGSBASE-Enable Bit.
#define CR4_PCIDE           BS(17)   // PCID-Enable Bit.
#define CR4_OSXSAVE         BS(18)   // XSAVE and Processor Extended States-Enable Bit.
#define CR4_SMEP            BS(20)   // SMEP-Enable Bit.
#define CR4_SMAP            BS(21)   // SMAP-Enable Bit.

typedef struct cpu {
    long            ncli;
    long            intena;
    uint64_t        version;

    long            apicID;
    long            freq;
    uint64_t        flags;
    uint64_t        features;
    uint64_t        timer_ticks;
    
    tss_t           tss;
    gdt_t           gdt;
    
    context_t       *ctx;
    void            *math_ctx;

    thread_t        *thread;
    thread_t        *simd_thread;
    sched_queue_t    queueq;

    uint8_t         phys_addrsz;
    uint8_t         virt_addrsz;

    char            vendor[16];
    char            brand_string[64];
} cpu_t;


#define CPU_ENABLED         BS(0)   // cpu is usable(enabled).
#define CPU_ISBSP           BS(1)   // cpu is a bootstrap processor. 
#define CPU_ONLINE          BS(2)   // cpu is online.
#define CPU_64BIT           BS(3)   // cpu running in 64bit.
#define CPU_USE_LAPIC       BS(4)   // cpu will use the Local APIC.

#define CPU_PANICED         BS(31)  // cpu has paniced.

#define MAXNCPU             16 // maximum supported cpus
extern cpu_t                *cpus[MAXNCPU];

#define cpu                 (getcls())                      // get CPU local structure.
#define current             (cpu->thread)                   // currently running thread.
#define simd_thread         (cpu->simd_thread)              // current SIMD/FPU thread.
#define simdctx             (cpu->math_ctx)                 // current SIMD/FPU context.
#define ready_queue         (cpu->queueq)                   // per-CPU ready queue.
#define cpu_setflags(f)     (cpu->flags | (f))              // set CPU flag(s).
#define cpu_testflags(f)    (cpu->flags & (f))              // test CPU flag(s).
#define cpu_features        (cpu->features)
#define cpu_has(f)          (cpu_features & (f))            // Check for CPU features (read-only).
#define isbsp()             (cpu_testflags(CPU_ISBSP))      // is this CPU a bootstrap processor?

extern int                  sse_init(void);
extern void                 simd_fp_except(void);
extern void                 coprocessor_except(void);

extern int                  is64bit(void);
extern void                 cpu_init(void);
extern int                  cpu_rsel(void);
extern int                  cpu_count(void);
extern int                  cpu_online(void);
extern void                 cpu_incr_online(void);

extern int                  bootothers(void);

extern cpu_t                *getcls(void);
extern void                 setcls(cpu_t *c);

extern int                  getcpuid(void);
extern int                  enumerate_cpus(void);
extern void                 cpu_get_features(void);