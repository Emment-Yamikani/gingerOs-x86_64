#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>

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

#define loop()                          for (;;)

#define forlinked(elem, list, iter) \
    for (typeof(list) elem = list; elem; elem = iter)

#define foreach(elem, list)                         \
    for (typeof(*list) *tmp = list,                 \
        elem = (typeof(elem))(tmp ? *tmp : NULL);   \
        elem; elem = *++tmp)

#ifndef container_of
#define container_of(ptr, type, member) ({ \
	const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - offsetof(type,member) ); })
#endif

#define BCD2binary(bcd)         ((((bcd) & 0xF0) >> 1) + (((bcd) & 0xF0) >> 3) + ((bcd) & 0xf))

#define __retaddr(l)            __builtin_return_address(l)

#define NOT(a)                  (~(uintptr_t)(a))                   // bit-wise NOT().
#define BS(p)                   ((uintptr_t)(1) << (p))             // bit-wise SHL().
#define AND(a, b)               ((uintptr_t)(a) & (uintptr_t)(b))   // bit-wise AND().
#define OR(a, b)                ((uintptr_t)(a) | (uintptr_t)(b))   // bit-wise OR().
#define XOR(a, b)               ((uintptr_t)(a) ^ (uintptr_t)(b))   // bit-wise XOR().
#define NAND(a, b)              (AND(NOT((a)), (b)))                // bit-wise NAND().
#define SHL(a, b)               ((uintptr_t)(a) <<(uintptr_t)(b))   // bit-wise SHL().
#define SHR(a, b)               ((uintptr_t)(a) >>(uintptr_t)(b))   // bit-wise SHR().
#define BTEST(a, b)             (AND((a), BS(b)))                   // test a bit in a bitset.
#define MAX(a, b)               ((long)((a) > (b) ? (a) : (b)))     // take max of two arguments
#define MIN(a, b)               ((long)((a) < (b) ? (a) : (b)))     // take min of two arguments.
#define ABS(a)                  (((long)(a) < 0) ? -(long)(a) : (a))// take the absolute value of an argument.
#define ABSi(x)                 ({ (int)x < 0 ? -(int)(x) : (int)(x); })

// C('A') == Control-A
#define CTRL(x)                 ((x) - '@')

#define KiB(x)                  ((size_t)(1024ul * ((size_t)(x))))
#define MiB(x)                  ((size_t)(KiB(1024ul) * ((size_t)(x))))
#define GiB(x)                  ((size_t)(MiB(1024ul) * ((size_t)(x))))
#define PGSZ                    (0x1000ul)
#define PAGESZ                  (PGSZ)
#define PGMASK                  (PGSZ - 1)
#define PAGEMASK                (PGMASK)
#define PGSZ2M                  (0x200000ul)
#define PGSZ2MASK               (PGSZ2M -1)
#define ALIGN16(x)              (AND((uintptr_t)(x), NOT(0xf)))
#define ALIGN4K(x)              (AND((uintptr_t)(x), NOT(PGMASK)))
#define PGALIGN(x)              (ALIGN4K(x))

#define MEMMDEV                 ((uintptr_t)0xFE000000ul)

#define ismmio_addr(x)          ((((uintptr_t)(x)) >= MEMMDEV) && (((uintptr_t)(x)) < GiB(4)))

#if defined __i386__
    #define USTACK              ((uintptr_t)0xC0000000ul)
#elif defined __x86_64__
    #define USTACK              ((uintptr_t)0x800000000000ul)
#endif

#if defined __i386__
    #define VMA_BASE            ((uintptr_t)0xC0000000ul)
#elif defined __x86_64__
    #define VMA_BASE            ((uintptr_t)0xFFFFFF8000000000ul)
#endif

#define VMA(x)                  (VMA_BASE + (uintptr_t)(x))
#define VMA2HI(p)               (VMA_BASE + (uintptr_t)(p))
#define VMA2LO(x)               ((uintptr_t)(x) - VMA_BASE)

#define iskernel_addr(x)        ((uintptr_t)(x) >= VMA_BASE)

#define PGOFF(p)                (AND((uintptr_t)(p), PGMASK))

#define PG2MOFF(p)              (AND((uintptr_t)(p), PGSZ2MASK))

#define PGROUND(p)              ((uintptr_t)AND(((uintptr_t)(p)), ~PGMASK))

#define PG2MROUND(p)            ((uintptr_t)AND(((uintptr_t)(p)), ~PGSZ2MASK))

#define NPAGE(p)                (((size_t)(p) / PGSZ) + (PGOFF(p) ? 1 : 0))

#define N2MPAGE(p)              (((size_t)(p) / PGSZ2M) + (PG2MOFF(p) ? 1 : 0))

#define PGROUNDUP(p)            (PGOFF(p) ? (PGROUND(((uintptr_t)p) + PAGESZ) ) : (uintptr_t)(p))

#define PG2MROUNDUP(p)          (PG2MOFF(p) ? (PGROUND(((uintptr_t)p) + PGSZ2M)) : (uintptr_t)(p))

#define ALIGN4KUP(p)            (PGROUNDUP(p))

#define NELEM(x)                ((size_t)(sizeof ((x)) / sizeof ((x)[0])))

extern void _kernel_end();
extern void _kernel_start();