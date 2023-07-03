#pragma once

#include <lib/stdint.h>

uintptr_t rdrflags(void);
void wrrflags(uintptr_t);

static inline void hlt() { asm __volatile__ ("hlt"); }
static inline void cli() { asm __volatile__ ("cli"); }
static inline void sti() { asm __volatile__ ("sti"); }
static inline void pause() { asm __volatile__("pause"); }

static inline uintptr_t rdrax(void) {uintptr_t ret; asm volatile("":"=a"(ret)); return ret;}

void disable_caching(void);

void wrcr0(uint64_t);
uint32_t rdcr0(void);

void wrcr2(uintptr_t);
uintptr_t rdcr2(void);

void wrcr3(uintptr_t);
uintptr_t rdcr3(void);

uint32_t rdcr4(void);
void wrcr4(uint64_t);

//uintptr_t rdrax(void);

void cpuid(uint64_t func, uint64_t subfunc, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);

extern void invlpg(uintptr_t);

#define is_intena() ({uint64_t intena = rdrflags() & 0x200; intena;})

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

/*
static inline void
loadgs(uint16_t v)
{
    asm volatile("movw %0, %%gs"
                 :
                 : "r"(v));
}*/