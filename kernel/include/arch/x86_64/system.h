#pragma once

#include <lib/cpuid.h>
#include <lib/stdint.h>
#include <lib/types.h>

// halt the current processor core.
static inline void hlt(void) {
#if defined __i386__ || __x86_64__
    asm __volatile__ ("hlt");
#endif
}

// clear interrupts on the current processor core.
static inline void cli(void) {
#if defined __i386__ || __x86_64__
    asm __volatile__ ("cli");
#endif
}

// start interrupts on the current processor core.
static inline void sti(void) {
#if defined __i386__ || __x86_64__
    asm __volatile__ ("sti");
#endif
}

// momentarily pause the current processor core.
static inline void cpu_pause(void) {
#if defined __i386__ || __x86_64__
    asm __volatile__("pause");
#endif
}

extern void disable_caching(void);

static inline uintptr_t rdrax(void) {
    uintptr_t ret;
    asm volatile("":"=a"(ret));
    return ret;
}

//uintptr_t rdrax(void);
extern void wrcr0(u64);
extern u64 rdcr0(void);
extern u64 rdcr4(void);
extern void wrcr4(u64);
extern uintptr_t rdcr2(void);
extern uintptr_t rdcr3(void);
extern uintptr_t rdrsp(void);
extern uintptr_t rdrbp(void);
extern void cr0set(u64);
extern void cr4set(u64);
extern void wrcr2(uintptr_t);
extern void wrcr3(uintptr_t);
extern void cr0mask(u64);
extern void cr4mask(u64);
extern uintptr_t rdrflags(void);
extern void wrrflags(uintptr_t);
extern u64 cr0test(u64 bits);
extern u64 cr4test(u64 bits);

static inline void cpuid(u64 leaf, u64 subleaf, u32 *eax,
                  u32 *ebx, u32 *ecx, u32 *edx) {
    __get_cpuid_count(leaf, subleaf, eax, ebx, ecx, edx);
}

extern u64 rdxcr(u32 i);
extern void wrxcr(u32 i, u64 value);
extern void xsave(void *region);
extern void xrstor(void *region);
extern void fxsave(void *region);
extern void fxrstor(void *region);

extern void fninit(void);
extern void invlpg(uintptr_t);

#define is_intena() ({ (rdrflags() & 0x200); })

static inline u8 inb(u16 port) {
    u8 data;
#if defined __i386__ || __x86_64__    
    __asm__ __volatile__("inb %1, %0"
                         : "=a"(data)
                         : "dN"(port));
#endif
    return data;
}

static inline void outb(u16 port, u8 data) {
#if defined __i386__ || __x86_64__    
    __asm__ __volatile__("outb %1, %0"
                         :
                         : "dN"(port), "a"(data));
#endif
}