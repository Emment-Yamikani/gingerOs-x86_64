#ifndef PMM_H
#define PMM_H 1

#include <lib/stdint.h>
#include <lib/stddef.h>
#include <sync/atomic.h>
#include <sync/spinlock.h>
#include <mm/gfp.h>

// physical memory manager entries
struct pmman {
    int         (*init)(void);    // initialize the physical memory manager.
    uintptr_t   (*alloc)(void);   // allocate a 4K page.
    void        (*free)(uintptr_t);   // free a 4K page.
    int         (*get_page)(gfp_t gfp, void **ppa);
    int         (*get_pages)(gfp_t gfp, size_t order, void **ppa);
    size_t      (*mem_used)(void); // used space (in KBs).
    size_t      (*mem_free)(void); // free space (in KBs).
};

extern struct pmman pmman;

int physical_memory_init(void);

#endif //PMM_H