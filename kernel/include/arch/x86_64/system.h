#pragma once

#include <lib/stdint.h>

static inline void hlt() { asm __volatile__ ("hlt"); }
static inline void cli() { asm __volatile__ ("cli"); }
static inline void sti() { asm __volatile__ ("sti"); }
static inline void pause() { asm __volatile__("pause"); }

extern void disable_caching(void);
static inline uintptr_t rdrax(void) {uintptr_t ret; asm volatile("":"=a"(ret)); return ret;}

//uintptr_t rdrax(void);
extern void wrcr0(uint64_t);
extern uint64_t rdcr0(void);
extern uint64_t rdcr4(void);
extern void wrcr4(uint64_t);
extern uintptr_t rdcr2(void);
extern uintptr_t rdcr3(void);
extern uintptr_t rdrsp(void);
extern uintptr_t rdrbp(void);
extern void cr0set(uint64_t);
extern void cr4set(uint64_t);
extern void wrcr2(uintptr_t);
extern void wrcr3(uintptr_t);
extern void cr0mask(uint64_t);
extern void cr4mask(uint64_t);
extern uintptr_t rdrflags(void);
extern void wrrflags(uintptr_t);
extern uint64_t cr0test(uint64_t bits);
extern uint64_t cr4test(uint64_t bits);

extern void cpuid(uint64_t func, uint64_t subfunc, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);

extern uint64_t rdxcr(uint32_t i);
extern void wrxcr(uint32_t i, uint64_t value);
extern void xsave(void *region);
extern void xrstor(void *region);
extern void fxsave(void *region);
extern void fxrstor(void *region);

extern void finit(void);
extern void invlpg(uintptr_t);

#define is_intena() ({ (rdrflags() & 0x200); })

static inline uint8_t inb(uint16_t port)
{
    uint8_t data;
    __asm__ __volatile__("inb %1, %0"
                         : "=a"(data)
                         : "dN"(port));
    return data;
}

static inline void outb(uint16_t port, uint8_t data)
{
    __asm__ __volatile__("outb %1, %0"
                         :
                         : "dN"(port), "a"(data));
}