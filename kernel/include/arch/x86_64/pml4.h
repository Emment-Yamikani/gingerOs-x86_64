#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>
#include <sys/system.h>
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

typedef union {
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

extern pte_t __pml4[512] __aligned(0x1000);

typedef union {
    struct {
        uint64_t off : 12;
        uint64_t pti : 9;
        uint64_t pdi : 9;
        uint64_t pdpti : 9;
        uint64_t pml4i : 9;
        uint64_t resvd : 16;
    };
    uint64_t raw;
} viraddr_t;

typedef struct pagemap {
    pte_t *pml4;
    unsigned flags;
    spinlock_t lock;    // 
} pagemap_t;

extern pagemap_t kernel_map;

#define __viraddr(pml4e, pdpte, pde, pte) ((viraddr_t){ \
    .pml4i = (pml4e),                                   \
    .pdpti = (pdpte),                                   \
    .pdi = (pde),                                       \
    .pti = (pte),                                       \
}                                                       \
                                               .raw)

#define PTI(v)                      ((viraddr_t){.raw = (v)}.pti)
#define PDI(v)                      ((viraddr_t){.raw = (v)}.pdi)
#define PDPTI(v)                    ((viraddr_t){.raw = (v)}.pdpti)
#define PML4I(v)                    ((viraddr_t){.raw = (v)}.pml4i)

#define LVL_PTE                     1
#define LVL_PDTE                    2
#define LVL_PDPTE                   3
#define LVL_PML4E                   4

extern spinlock_t kmap_lk; // global kernel address space lock
#define kmap_lock()                 ({spin_lock(&kmap_lk);})
#define kmap_unlock()               ({spin_unlock(&kmap_lk);})
#define kmap_locked()               ({spin_locked(&kmap_lk);})
#define kmap_assert_locked()        ({spin_assert_locked(&kmap_lk);})

#define pagemap_assert(map)         ({ assert(map, "%s:%d: error: pagemap not specified\n"); })
#define pagemap_lock(map)           ({ pagemap_assert(map); spin_lock(&(map)->lock); })
#define pagemap_unlock(map)         ({ pagemap_assert(map); spin_unlock(&(map)->lock); })
#define pagemap_assert_locked(map)  ({ pagemap_assert(map); spin_assert_locked(&(map)->lock); })

/// guarantees locks on both (pagemap_t *) and kmap portion of pagemap_t before procesding
#define pagemap_binary_lock(map)    ({ pagemap_lock(map); kmap_lock(); })

// releases both (pagemap_t*)->lock and kmap_lk locks 
#define pagemap_binary_unlock(map)  ({ kmap_unlock(); pagemap_unlock(map); })

// is page a user page
#define isuser_page(flags)          ((flags)&VM_U)
// is a 2mb page?
#define is2mb_page(flags)           ((flags)&VM_PS)

#define GETPHYS(entry)              ((uintptr_t)((entry) ? PGROUND((entry)->raw) : 0))

#define ALLOC_PAGE                  0x800
// remap page
#define REMAPPG                     0x1000

#define isalloc_page(flags)         ((flags)&ALLOC_PAGE)

void pagemap_resolve(void);
void pagemap_free(pagemap_t *map);
int pagemap_switch(pagemap_t *map);
int pagemap_alloc(pagemap_t **pmap);
int unmap_page(pagemap_t *map, uintptr_t v);
pte_t *get_mapping(pagemap_t *map, uintptr_t v);
int map_page(pagemap_t *map, uintptr_t v, size_t sz, uint32_t flags);
int unmap_page_n(pagemap_t *map, uintptr_t v, size_t size, uint32_t flags);
int map_page_to(pagemap_t *map, uintptr_t v, uintptr_t paddr, uint32_t flags);
int map_page_to_n(pagemap_t *map, uintptr_t v, uintptr_t p, size_t sz, uint32_t flags);
int unmap_table_entry(pagemap_t *map, int level, int pml4i, int pdpti, int pdi, int pti);
