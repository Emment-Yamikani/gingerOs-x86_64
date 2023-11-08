#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>
#include <sys/system.h>
#include <arch/pagemap.h>
#include <sync/spinlock.h>

#define VM_P        (BS(0))             // page is present.
#define VM_W        (BS(1))             // page is writtable.
#define VM_U        (BS(2))             // page is user accesible.
#define VM_PWT      (BS(3))             // page write through.
#define VM_PCD      (BS(4))             // page cache disabled.
#define VM_A        (BS(5))             // page is accessed.
#define VM_D        (BS(6))             // page is dirty.
#define VM_PS       (BS(7))             // page size.
#define VM_G        (BS(8))             // page is global.
#define VM_PCDWT    (VM_PCD | VM_PWT)   // page level caching disabled and write through enabled.

#define VM_R        (VM_P)
#define VM_KR       (VM_R)
#define VM_KW       (VM_KR | VM_W)
#define VM_KRW      (VM_KR | VM_KW)
#define VM_UR       (VM_U  | VM_R)
#define VM_URW      (VM_UR | VM_W)

#define VM_2MB      (VM_PS)
#define VM_K2MBR    (VM_KR  | VM_2MB)
#define VM_K2MBRW   (VM_KRW | VM_2MB)

#define VM_U2MBR    (VM_UR  | VM_2MB)
#define VM_U2MBRW   (VM_URW | VM_2MB)

typedef union pte {
    struct {
        uint64_t p : 1;
        uint64_t w : 1;
        uint64_t u : 1;
        uint64_t pwt : 1;
        uint64_t pcd : 1;
        uint64_t a : 1;
        uint64_t d : 1;
        uint64_t ps : 1;
        uint64_t g : 1;
        uint64_t ign1 : 2;
        uint64_t alloc : 1;
        uint64_t phys : 40;
        uint64_t ign2 : 12;
    };
    uint64_t raw;
} __packed pte_t;

extern pte_t _PML4_[512] __aligned(0x1000);

typedef union viraddr {
    struct {
        uint64_t off : 12;
        uint64_t pti : 9;
        uint64_t pdi : 9;
        uint64_t pdpti : 9;
        uint64_t pml4i : 9;
        uint64_t resvd : 16;
    };
    uint64_t raw;
} __packed viraddr_t;

#define __viraddr(pml4e, pdpte, pde, pte) ((viraddr_t){ \
    .pdi = (pde),                                       \
    .pti = (pte),                                       \
    .pml4i = (pml4e),                                   \
    .pdpti = (pdpte),                                   \
}.raw)

#define PTI(v)                  ((viraddr_t){.raw = (uintptr_t)(v)}.pti)
#define PDI(v)                  ((viraddr_t){.raw = (uintptr_t)(v)}.pdi)
#define PDPTI(v)                ((viraddr_t){.raw = (uintptr_t)(v)}.pdpti)
#define PML4I(v)                ((viraddr_t){.raw = (uintptr_t)(v)}.pml4i)

#define LVL_PTE                 1
#define LVL_PDTE                2
#define LVL_PDPTE               3
#define LVL_PML4E               4

#define NPTE                    (512)
#define iL_INV(i)               (((i) < 0) || ((i) >= NPTE))

/**
 * PML4 -> 0xFFFFFF7FBFDFE000
 * PDPT -> 0xFFFFFF7FBFC00000
 * PDT  -> 0xFFFFFF7F80000000
 * PT   -> 0xFFFFFF0000000000
 */

#define PML4                    ((pte_t *)(0xFFFFFF7FBFDFE000ull))
#define PDPT(PDPi)              ((pte_t *)(0xFFFFFF7FBFC00000ull + (0x1000ul * ((size_t)PDPi))))
#define PDT(PDPi, PDi)          ((pte_t *)(0xFFFFFF7F80000000ull + (0x200000ul * ((size_t)PDPi)) + (0x1000ul * ((size_t)PDi)) ))
#define PT(PDPi, PDi, PTi)      ((pte_t *)(0xFFFFFF0000000000ull + (0x40000000ul * ((size_t)PDPi)) + (0x200000ul * ((size_t)PDi)) + (0x1000ul * ((size_t)PTi))))

#define PML4E(i4)               ({ &PML4[i4]; })
#define PDPTE(i4, i3)           ({ &PDPT(i4)[i3]; })
#define PDTE(i4, i3, i2)        ({ &PDT(i4, i3)[i2]; })
#define PTE(i4, i3, i2, i1)     ({ &PT(i4, i3, i2)[i1]; })

#define ispresent(flags)        ((flags)&VM_P)
#define iswritable(flags)       ((flags)&VM_W)
#define isuser_page(flags)      ((flags)&VM_U)      // is page a user page?
#define is2mb_page(flags)       ((flags)&VM_PS)     // is a 2mb page?
#define isPS(flags)             ((flags) & VM_PS)   // is page size flags set?

#define GETPHYS(entry)          ((uintptr_t)((entry) ? PGROUND((entry)->raw) : 0))

#define ALLOC_PAGE              0x800   // page was allocated.
#define REMAPPG                 0x1000  // remap page

#define isalloc_page(flags)     ((flags)&ALLOC_PAGE)

/**
 * 
*/
int i64_swtchvm(uintptr_t pdbr, uintptr_t *old);

/**
 * 
*/
int i64_map(uintptr_t frame, int i4, int i3, int i2, int i1, int flags);

/**
 * 
*/
void i64_unmap(int i4, int i3, int i2, int i1);

/**
 * 
*/
void i64_unmap_n(uintptr_t v, size_t sz);

/**
 * 
*/
int i64_map_i(uintptr_t v, uintptr_t p, size_t sz, int flags);

/**
 * 
*/
int i64_map_n(uintptr_t v, size_t sz, int flags);

/**
 * 
*/
int i64_mount(uintptr_t p, void **pvp);

/**
 * 
*/
void i64_unmount(uintptr_t v);

/**
 * 
*/
void i64_unmap_full(void);

/**
 * 
*/
void i64_fullvm_unmap (uintptr_t pml4);

/**
 * 
*/
int i64_lazycpy(uintptr_t dst, uintptr_t src);

/**
 * 
*/
int i64_memcpypp(uintptr_t pdst, uintptr_t psrc, size_t size);

/**
 * 
*/
int i64_memcpyvp(uintptr_t p, uintptr_t v, size_t size);

/**
 * 
*/
int i64_memcpypv(uintptr_t v, uintptr_t p, size_t size);

/**
 * 
*/
int i64_getmapping(uintptr_t addr, pte_t **pte);

/**
 * 
*/
int i64_pml4alloc(uintptr_t *ref);
