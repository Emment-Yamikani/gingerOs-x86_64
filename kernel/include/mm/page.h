#pragma once

#include <fs/icache.h>
#include <lib/stdint.h>
#include <lib/stddef.h>
#include <lib/types.h>
#include <mm/mm_gfp.h>
#include <mm/page_flags.h>
#include <sync/assert.h>

typedef struct page {
    u64             flags;
    atomic_t        refcnt;
    atomic_t        mapcnt;
    icache_t        *icache;
    uintptr_t       virtual; // virtual addr
} __packed page_t;

#define page_resetflags(page)       ({ (page)->flags = 0; })
#define page_testflags(page, f)     ({ (page)->flags & (f); })                       // get page flags.
#define page_setflags(page, f)      ({ (page)->flags |= (f); })
#define page_maskflags(page, f)     ({ (page)->flags &= ~(f); })

#define page_isexec(page)           ({ page_testflags(page, PG_EXEC); })     // exec'able.
#define page_iswrite(page)          ({ page_testflags(page, PG_WRITE); })    // writeable.
#define page_isread(page)           ({ page_testflags(page, PG_READ); })     // readable.
#define page_isuser(page)           ({ page_testflags(page, PG_USER); })     // user.
#define page_isvalid(page)          ({ page_testflags(page, PG_VALID); })    // valid.
#define page_isdirty(page)          ({ page_testflags(page, PG_DIRTY); })    // dirty.
#define page_isshared(page)         ({ page_testflags(page, PG_SHARED); })
#define page_iswriteback(page)      ({ page_testflags(page, PG_WRITEBACK); })
#define page_isswapped(page)        ({ page_testflags(page, PG_SWAPPED); })  // swapped.
#define page_isswappable(page)      ({ page_testflags(page, PG_SWAPPABLE); })// canswap.

#define page_setrx(page)            ({ page_setflags(page, PG_RX); })
#define page_setrw(page)            ({ page_setflags(page, PG_RW); })
#define page_setrwx(page)           ({ page_setflags(page, PG_RWX); })
#define page_setuser(page)          ({ page_setflags(page, PG_USER); })          // set 'user'.
#define page_setexec(page)          ({ page_setflags(page, PG_EXEC); })          // set 'exec'able'.
#define page_setread(page)          ({ page_setflags(page, PG_READ); })          // set 'readable'.
#define page_setwrite(page)         ({ page_setflags(page, PG_WRITE); })         // set 'writeable'.
#define page_setdirty(page)         ({ page_setflags(page, PG_DIRTY); })         // set 'dirty'.
#define page_setvalid(page)         ({ page_setflags(page, PG_VALID); })         // set 'valid'.
#define page_setshared(page)        ({ page_setflags(page, PG_SHARED); })
#define page_setwriteback(page)     ({ page_setflags(page, PG_WRITEBACK); })
#define page_setswapped(page)       ({ page_setflags(page, PG_SWAPPED); })       // set 'swapped'.
#define page_setswappable(page)     ({ page_setflags(page, PG_SWAPPABLE); })     // set 'swappable'.

#define page_maskrx(page)           ({ page_maskflags(page, PG_RX); })
#define page_maskrw(page)           ({ page_maskflags(page, PG_RW); })
#define page_maskrwx(page)          ({ page_maskflags(page, PG_RWX); })
#define page_maskuser(page)         ({ page_maskflags(page, PG_USER); })          // set 'user'.
#define page_maskexec(page)         ({ page_maskflags(page, PG_EXEC); })          // set 'exec'able'.
#define page_maskread(page)         ({ page_maskflags(page, PG_READ); })          // set 'readable'.
#define page_maskwrite(page)        ({ page_maskflags(page, PG_WRITE); })         // set 'writeable'.
#define page_maskdirty(page)        ({ page_maskflags(page, PG_DIRTY); })         // set 'dirty'.
#define page_maskvalid(page)        ({ page_maskflags(page, PG_VALID); })         // set 'valid'.
#define page_maskshared(page)       ({ page_maskflags(page, PG_SHARED); })
#define page_maskwriteback(page)    ({ page_maskflags(page, PG_WRITEBACK); })
#define page_maskswapped(page)      ({ page_maskflags(page, PG_SWAPPED); })       // set 'swapped'.
#define page_maskswappable(page)    ({ page_maskflags(page, PG_SWAPPABLE); })     // set 'swappable'.

#define page_refcnt(page)           ({ (page)->refcnt; })
#define page_virtual(page)          ({ (page)->virtual; })

void page_free_n(page_t *page, usize order);
void __page_free_n(uintptr_t paddr, usize order);

void page_free(page_t *page);
void __page_free(uintptr_t paddr);

int page_alloc_n(gfp_mask_t gfp, usize order, page_t **pp);
int __page_alloc_n(gfp_mask_t gfp, usize order, void **pp);

int page_alloc(gfp_mask_t gfp, page_t **pp);
int __page_alloc(gfp_mask_t gfp, void **pp);

int page_increment(page_t *page);
int __page_increment(uintptr_t paddr);

int page_getref(page_t *page);
int __page_getref(uintptr_t paddr);

int page_decrement(page_t *page);
int __page_decrement(uintptr_t paddr);

void page_putref(page_t *page);
int __page_putref(uintptr_t paddr);

int page_get_address(page_t *page, void **ppa);

int page_getcount(page_t *page, usize *pcnt);
int __page_getcount(uintptr_t paddr, usize *pcnt);