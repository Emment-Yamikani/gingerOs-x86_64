#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>
#include <sys/system.h>
#include <sync/spinlock.h>
#include <lib/types.h>

#define PTE_P        BS(0)  // page is present.
#define PTE_W        BS(1)  // page is writtable.
#define PTE_U        BS(2)  // page is user accesible.
#define PTE_WT       BS(3)  // page write through.
#define PTE_CD       BS(4)  // page cache disabled.
#define PTE_A        BS(5)  // page is accessed.
#define PTE_D        BS(6)  // page is dirty.
#define PTE_PS       BS(7)  // page size.
#define PTE_G        BS(8)  // page is global.
#define PTE_ZERO     BS(9)  // is to be zero before allocation?, (NB: cleared when setting paging-structures).
#define PTE_REMAP    BS(10) // remap page-table entry? (NB: also like PTE_ZERO, cleared when setting paging-structures).
#define PTE_ALLOC    BS(11) // page frame was implicitly allocated?.


#define PTE_X        (PTE_P /*| BS(64)*/)// page is executable?
#define PTE_WTCD     (PTE_WT | PTE_CD) // page level caching disabled and write through enabled.

#define PTE_R        (PTE_P)
#define PTE_KR       (PTE_R)
#define PTE_KW       (PTE_KR | PTE_W)
#define PTE_KRW      (PTE_KR | PTE_KW)
#define PTE_UR       (PTE_U  | PTE_R)
#define PTE_URW      (PTE_UR | PTE_W)

#define _isP(f)                 ((f) & PTE_P)
#define _isX(f)                 ((f) & PTE_X)
#define _isW(f)                 ((f) & PTE_W)
#define _isR(f)                 ((f) & PTE_R)
#define _isU(f)                 ((f) & PTE_U)
#define _isPS(f)                ((f) & PTE_PS)
#define _iszero(f)              ((f) & PTE_ZERO)
#define _isremap(f)             ((f) & PTE_REMAP)
#define _isalloc(f)             ((f) & PTE_ALLOC)

#define pte_isP(pte)        (_isP((pte)->raw))      // is present?
#define pte_isW(pte)        (_isW((pte)->raw))      // is writable?
#define pte_isU(pte)        (_isU((pte)->raw))      // is a user page?
#define pte_isPS(pte)       (_isPS((pte)->raw))     // is page size flags set?
#define pte_isalloc(pte)    (_isalloc((pte)->raw))  // is allocated?

typedef union pte {
    struct {
        u64 p       : 1;
        u64 w       : 1;
        u64 u       : 1;
        u64 pwt     : 1;
        u64 pcd     : 1;
        u64 a       : 1;
        u64 d       : 1;
        u64 ps      : 1;
        u64 g       : 1;
        u64 ign1    : 2;
        u64 alloc   : 1;
        u64 phys    : 40;
        u64 ign2    : 12;
        //011
    };
    u64 raw;
} __packed pte_t;

extern pte_t _PML4_[512] __aligned(0x1000);

typedef union viraddr {
    struct {
        u64 off     : 12;
        u64 pti     : 9;
        u64 pdi     : 9;
        u64 pdpti   : 9;
        u64 pml4i   : 9;
        u64 resvd   : 16;
    };
    u64 raw;
} __packed viraddr_t;

// page indices to a virtual address.
#define i2v(i4, i3, i2, i1) ((viraddr_t){  \
    .off    = 0,                           \
    .pti    = (i1),                        \
    .pdi    = (i2),                        \
    .pdpti  = (i3),                        \
    .pml4i  = (i4),                        \
    .resvd  = (((i4) >= 256) ? 0xFFFFu : 0)}\
.raw)

#define PML4I(p)                (((u64)(p) >> 39) & 0x1FFu)
#define PDPTI(p)                (((u64)(p) >> 30) & 0x1FFu)
#define PDI(p)                  (((u64)(p) >> 21) & 0x1FFu)
#define PTI(p)                  (((u64)(p) >> 12) & 0x1FFu)

#define NPTE                    512
#define iL_INV(i)               (((i) < 0) || ((i) >= NPTE))

#define PML4                    ((pte_t *)(0xFFFFFFFFFFFFF000ull))
#define PDPT(i4)                ((pte_t *)(0xFFFFFFFFFFE00000ull + (PGSZ * (u64)(i4))))
#define PDT(i4, i3)             ((pte_t *)(0xFFFFFFFFC0000000ull + (PGSZ2MB * (u64)(i4)) + (PGSZ * (u64)(i3))))
#define PT(i4, i3, i2)          ((pte_t *)(0xFFFFFF8000000000ull + (PGSZ1GB * (u64)(i4)) + (PGSZ2MB * (u64)(i3)) + (PGSZ * (u64)(i2))))

#define PML4E(i4)               ({ &PML4[i4]; })
#define PDPTE(i4, i3)           ({ &PDPT(i4)[i3]; })
#define PDTE(i4, i3, i2)        ({ &PDT(i4, i3)[i2]; })
#define PTE(i4, i3, i2, i1)     ({ &PT(i4, i3, i2)[i1]; })

// extract flags used to map a page.
#define extract_vmflags(flags)  ({ PGOFF(flags); })

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
void x86_64_unmap_n(uintptr_t v, usize sz);

/**
 * 
*/
int x86_64_map_i(uintptr_t v, uintptr_t p, usize sz, int flags);

/**
 * 
*/
int x86_64_map_n(uintptr_t v, usize sz, int flags);

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
 * @brief 
 * 
 */
int x86_64_mprotect(uintptr_t vaddr, usize sz, int flags);

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
int x86_64_memcpypp(uintptr_t pdst, uintptr_t psrc, usize size);

/**
 * 
*/
int x86_64_memcpyvp(uintptr_t p, uintptr_t v, usize size);

/**
 * 
*/
int x86_64_memcpypv(uintptr_t v, uintptr_t p, usize size);

/**
 * 
*/
int x86_64_getmapping(uintptr_t addr, pte_t **pte);

/**
 * 
*/
int x86_64_pml4alloc(uintptr_t *ref);

void x86_64_pml4free(uintptr_t pgdir);

void x86_64_dumptable(pte_t *table);