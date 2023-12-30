#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>
#include <sys/system.h>
#include <arch/pagemap.h>
#include <sync/spinlock.h>

#define PTE_P        (BS(0))             // page is present.
#define PTE_W        (BS(1))             // page is writtable.
#define PTE_U        (BS(2))             // page is user accesible.
#define PTE_PWT      (BS(3))             // page write through.
#define PTE_PCD      (BS(4))             // page cache disabled.
#define PTE_A        (BS(5))             // page is accessed.
#define PTE_D        (BS(6))             // page is dirty.
#define PTE_PS       (BS(7))             // page size.
#define PTE_G        (BS(8))             // page is global.
#define PTE_X        (PTE_P /*| BS(64)*/)// page is executable?
#define PTE_PCDWT    (PTE_PCD | PTE_PWT) // page level caching disabled and write through enabled.

#define PTE_R        (PTE_P)
#define PTE_KR       (PTE_R)
#define PTE_KW       (PTE_KR | PTE_W)
#define PTE_KRW      (PTE_KR | PTE_KW)
#define PTE_UR       (PTE_U  | PTE_R)
#define PTE_URW      (PTE_UR | PTE_W)

#define PTE_2MB      (PTE_PS)
#define PTE_K2MBR    (PTE_KR  | PTE_2MB)
#define PTE_K2MBRW   (PTE_KRW | PTE_2MB)

#define PTE_U2MBR    (PTE_UR  | PTE_2MB)
#define PTE_U2MBRW   (PTE_URW | PTE_2MB)

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

#define LVL_PTE                 (1)
#define LVL_PDTE                (2)
#define LVL_PDPTE               (3)
#define LVL_PML4E               (4)

#define NPTE                    (512)
#define iL_INV(i)               (((i) < 0) || ((i) >= NPTE))

/**
 * PML4 -> (0xFFFFFF7FBFDFE000)
 * PDPT -> (0xFFFFFF7FBFC00000)
 * PDT  -> (0xFFFFFF7F80000000)
 * PT   -> (0xFFFFFF0000000000)
 */

#define PML4                    ((pte_t *)(0xFFFFFF7FBFDFE000ull))
#define PDPT(PDPi)              ((pte_t *)(0xFFFFFF7FBFC00000ull + (0x1000ul * ((size_t)PDPi))))
#define PDT(PDPi, PDi)          ((pte_t *)(0xFFFFFF7F80000000ull + (0x200000ul * ((size_t)PDPi)) + (0x1000ul * ((size_t)PDi)) ))
#define PT(PDPi, PDi, PTi)      ((pte_t *)(0xFFFFFF0000000000ull + (0x40000000ul * ((size_t)PDPi)) + (0x200000ul * ((size_t)PDi)) + (0x1000ul * ((size_t)PTi))))

#define PML4E(i4)               ({ &PML4[i4]; })
#define PDPTE(i4, i3)           ({ &PDPT(i4)[i3]; })
#define PDTE(i4, i3, i2)        ({ &PDT(i4, i3)[i2]; })
#define PTE(i4, i3, i2, i1)     ({ &PT(i4, i3, i2)[i1]; })

#define PTE_ALLOC_PAGE          0x800   // page was allocated.
#define PTE_REMAPPG             0x1000  // remap page.

#define _ispresent(flags)       ((flags) & PTE_P)
#define _iswritable(flags)      ((flags) & PTE_W)
#define _isuser_page(flags)     ((flags) & PTE_U)    // is page a user page?
#define _is2mb_page(flags)      ((flags) & PTE_PS)   // is a 2mb page?
#define _isPS(flags)            ((flags) & PTE_PS)   // is page size flags set?
#define _isalloc_page(flags)    ((flags) & PTE_ALLOC_PAGE) // page frame was allocated?.
#define _isremap(flags)         ((flags) & PTE_REMAPPG) // page remap(force remap) requested?.

#define pte_ispresent(pte)      (_ispresent((pte)->raw))
#define pte_iswritable(pte)     (_iswritable((pte)->raw))
#define pte_isuser_page(pte)    (_isuser_page((pte)->raw))    // is page a user page?
#define pte_is2mb_page(pte)     (_is2mb_page((pte)->raw))     // is a 2mb page?
#define pte_isPS(pte)           (_isPS((pte)->raw))           // is page size flags set?
#define pte_isalloc_page(pte)   (_isalloc_page((pte)->raw))

// extract flags used to map a page.
#define extract_vmflags(flags)  ({(PGOFF(flags) & ~PTE_REMAPPG); })

#define GETPHYS(entry)          ((uintptr_t)((entry) ? PGROUND((entry)->raw) : 0))


/**
 * 
*/
void x86_64_swtchvm(uintptr_t pdbr, uintptr_t *old);

/**
 * 
*/
int x86_64_map(uintptr_t frame, int i4, int i3, int i2, int i1, int flags);

/**
 * 
*/
void x86_64_unmap(int i4, int i3, int i2, int i1);

/**
 * 
*/
void x86_64_unmap_n(uintptr_t v, size_t sz);

/**
 * 
*/
int x86_64_map_i(uintptr_t v, uintptr_t p, size_t sz, int flags);

/**
 * 
*/
int x86_64_map_n(uintptr_t v, size_t sz, int flags);

/**
 * 
*/
int x86_64_mount(uintptr_t p, void **pvp);

/**
 * 
*/
void x86_64_unmount(uintptr_t v);

/**
 * 
*/
void x86_64_unmap_full(void);

/**
 * 
*/
void x86_64_fullvm_unmap (uintptr_t pml4);

/**
 * 
*/
int x86_64_lazycpy(uintptr_t dst, uintptr_t src);

/**
 * 
*/
int x86_64_memcpypp(uintptr_t pdst, uintptr_t psrc, size_t size);

/**
 * 
*/
int x86_64_memcpyvp(uintptr_t p, uintptr_t v, size_t size);

/**
 * 
*/
int x86_64_memcpypv(uintptr_t v, uintptr_t p, size_t size);

/**
 * 
*/
int x86_64_getmapping(uintptr_t addr, pte_t **pte);

/**
 * 
*/
int x86_64_pml4alloc(uintptr_t *ref);

void x86_64_pml4free(uintptr_t pgdir);