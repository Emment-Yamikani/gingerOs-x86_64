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

#define foreach(elem, list)                                             \
    for (typeof(*list) *__tmp__foreach = list,                          \
        elem = (typeof(elem))(__tmp__foreach ? *__tmp__foreach : NULL); \
        elem; elem = *++__tmp__foreach)

#define foreach_reverse(elem, list)                                     \
    for (typeof(*list) *__tmp__foreach = list,                          \
        elem = (typeof(elem))(__tmp__foreach ? *__tmp__foreach : NULL); \
        elem; elem = *--__tmp__foreach)

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
#define is_aligned2(p)          ((((u64)p) & 1) == 0)
#define is_aligned4(p)          ((((u64)p) & 3) == 0)
#define is_aligned8(p)          ((((u64)p) & 7) == 0)
#define is_aligned16(p)         ((((u64)p) & 0xf) == 0)
#define is_aligned32(p)         ((((u64)p) & 0x1f) == 0)
#define is_aligned64(p)         ((((u64)p) & 0x3f) == 0)

#define MEMMDEV                 ((uintptr_t)0xFE000000ul)

#define ismmio_addr(x)          ((((uintptr_t)(x)) >= MEMMDEV) && (((uintptr_t)(x)) < GiB(4)))

#define MAGIC_RETADDR           (-1ul)

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

extern int copy_to_user(void *udst, void *ksrc, size_t size);
extern int copy_from_user(void *kdst, void * usrc, size_t size);

void    swapi8(char  *dst, char *src);
void    swapi64(long *dst, long *src);
void    swapi32(int  *dst, int  *src);
void    swapi16(short *dst, short *src);

void    swapu8(unsigned char  *dst, unsigned char *src);
void    swapu64(unsigned long *dst, unsigned long *src);
void    swapu32(unsigned int  *dst, unsigned int  *src);
void    swapu16(unsigned short *dst, unsigned short *src);

void    swapptr(void **p0, void **p1);