#pragma once

//#include <ds/btree.h>
#include <fs/icache.h>
#include <lib/stdint.h>
#include <lib/stddef.h>
#include <lib/types.h>
#include <mm/page_flags.h>
#include <sync/assert.h>

typedef struct page {
    icache_t        *pg_mapping;
    atomic_t        pg_refcnt;
    atomic_t        pg_map_count;
    uintptr_t       pg_virtual; // virtual addr
    uint32_t        pg_flags;
    uint32_t        pg_mmzone;
} __packed page_t;

#define page_resetflags(page)       ({ (page)->pg_flags = 0; })
#define page_testflags(page, flags) ({ (page)->pg_flags & (flags); })                       // get page flags.
#define page_setflags(page, flags)  ({ (page)->pg_flags |= (flags); })
#define page_maskflags(page, flags) ({ (page)->pg_flags &= ~(flags); })

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

#define page_maskrx(page)            ({ page_maskflags(page, PG_RX); })
#define page_maskrw(page)            ({ page_maskflags(page, PG_RW); })
#define page_maskrwx(page)           ({ page_maskflags(page, PG_RWX); })
#define page_maskuser(page)          ({ page_maskflags(page, PG_USER); })          // set 'user'.
#define page_maskexec(page)          ({ page_maskflags(page, PG_EXEC); })          // set 'exec'able'.
#define page_maskread(page)          ({ page_maskflags(page, PG_READ); })          // set 'readable'.
#define page_maskwrite(page)         ({ page_maskflags(page, PG_WRITE); })         // set 'writeable'.
#define page_maskdirty(page)         ({ page_maskflags(page, PG_DIRTY); })         // set 'dirty'.
#define page_maskvalid(page)         ({ page_maskflags(page, PG_VALID); })         // set 'valid'.
#define page_maskshared(page)        ({ page_maskflags(page, PG_SHARED); })
#define page_maskwriteback(page)     ({ page_maskflags(page, PG_WRITEBACK); })
#define page_maskswapped(page)       ({ page_maskflags(page, PG_SWAPPED); })       // set 'swapped'.
#define page_maskswappable(page)     ({ page_maskflags(page, PG_SWAPPABLE); })     // set 'swappable'.

#define page_refcnt(page)           ({ (page)->pg_refcnt; })
#define page_virtual(page)          ({ (page)->pg_virtual; })
#define page_getmmzone(page)        ({ (page)->pg_mmzone; })
#define page_setmmzone(page, mm)    ({ (page)->pg_mmzone = (mm); })