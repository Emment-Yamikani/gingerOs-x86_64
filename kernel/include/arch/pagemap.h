#pragma once

#include <lib/types.h>
#include <lib/stdint.h>
#include <lib/stddef.h>
#include <sync/spinlock.h>
#include <arch/x86_64/types.h>

typedef struct pagemap {
    void        *pdbr;  // page directory base register.
    unsigned    flags;
    long        refcnt;
    spinlock_t  lock; //
} pagemap_t;

extern pagemap_t kernel_map;
extern spinlock_t kmap_lk; // global kernel address space lock
#define kmap_lock()                 ({spin_lock(&kmap_lk);})
#define kmap_unlock()               ({spin_unlock(&kmap_lk);})
#define kmap_locked()               ({spin_locked(&kmap_lk);})
#define kmap_assert_locked()        ({spin_assert_locked(&kmap_lk);})

#define pagemap_assert(map)         ({ assert(map, "%s:%d: error: pagemap not specified\n"); })
#define pagemap_lock(map)           ({ pagemap_assert(map); spin_lock(&(map)->lock); })
#define pagemap_unlock(map)         ({ pagemap_assert(map); spin_unlock(&(map)->lock); })
#define pagemap_assert_locked(map)  ({ pagemap_assert(map); spin_assert_locked(&(map)->lock); })

/**
 * guarantees locks on both (pagemap_t *) and kmap
 * portion of pagemap_t before procesding.
 */
#define pagemap_binary_lock(map)    ({ pagemap_lock(map); kmap_lock(); })

// releases both (pagemap_t*)->lock and kmap_lk locks 
#define pagemap_binary_unlock(map)  ({ kmap_unlock(); pagemap_unlock(map); })

void pagemap_resolve(void);