#pragma once

#include "../stdint.h"
#include "../stddef.h"

#define __CAT(a, b) a##b

#define __unused                        __attribute__((unused))
#define __packed                        __attribute__((packed))
#define __aligned(n)                    __attribute__((aligned(n)))
#define __noreturn                      __attribute__((noreturn))
#define __section(s)                    __attribute__((section(#s)))
#define __aligned_section(s, a)         __attribute__((section(#s), aligned(a)))
#define __fallthrough                   __attribute__((fallthrough))
#define __used_section(__section__)     __attribute__((used, section(#__section__)))
#define barrier()                       ({ asm volatile ("":::"memory"); })

#define loop()                           for (;;)

#define forlinked(elem, list, iter) \
    for (typeof(list) elem = list; elem; elem = iter)

#define foreach(elem, list) \
    for (typeof(*list) *tmp = list, elem = *tmp; elem; elem = *++tmp)

#ifndef container_of
#define container_of(ptr, type, member) ({ \
	const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - offsetof(type,member) ); })
#endif

#define BCD2binary(bcd) ((((bcd) & 0xF0) >> 1) + (((bcd) & 0xF0) >> 3) + ((bcd) & 0xf))

#define __retaddr(l) __builtin_return_address(l)

#define NOT(a)      (~(uintptr_t)(a))
#define BS(p)       ((uintptr_t)(1) << (p))
#define AND(a, b)   ((uintptr_t)(a) & (uintptr_t)(b))
#define OR(a, b)    ((uintptr_t)(a) | (uintptr_t)(b))
#define XOR(a, b)   ((uintptr_t)(a) ^ (uintptr_t)(b))
#define SHL(a, b)   ((uintptr_t)(a) <<(uintptr_t)(b))
#define SHR(a, b)   ((uintptr_t)(a) >>(uintptr_t)(b))
#define NAND(a, b)  (AND(NOT((a)), (b)))
#define BTEST(a, b) (AND((a), BS(b)))
#define MAX(a, b)   ((long)((a) > (b) ? (a) : (b)))
#define MIN(a, b)   ((long)((a) < (b) ? (a) : (b)))
#define ABS(a)      (((long)(a) < 0) ? -(long)(a) : (a))

// C('A') == Control-A
#define CTRL(x)     ((x) - '@')