#pragma once

#include <ds/btree.h>
#include <ds/queue.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <lib/types.h>
#include <sync/spinlock.h>

typedef struct page_cache_t page_cache_t;
typedef struct page_cache_t {
    long        pc_refcnt;
    uint32_t    pc_flags;
    inode_t     *pc_inode;
    btree_t     pc_btree;
    queue_t     pc_queue;
    size_t      pc_nrpages;
    spinlock_t  pc_lock;
} page_cache_t;

#define BTREE_INIT()    ((btree_t){0})
#define BTREE_NEW()     (&BTREE_INIT())

#define pgcache_assert(pgcache)               ({assert(pgcache, "No page cache"); })
#define pgcache_lock(pgcache)                 ({pgcache_assert(pgcache); spin_lock(&(pgcache)->pc_lock); })
#define pgcache_unlock(pgcache)               ({pgcache_assert(pgcache); spin_unlock(&(pgcache)->pc_lock); })
#define pgcache_islocked(pgcache)             ({pgcache_assert(pgcache); spin_islocked(&(pgcache)->pc_lock); })
#define pgcache_assert_locked(pgcache)        ({pgcache_assert(pgcache); spin_assert_locked(&(pgcache)->pc_lock); })

#define pgcache_btree(pgcache)                ({pgcache_assert_locked(pgcache); &(pgcache)->pc_btree; })
#define pgcache_queue(pgcache)                ({pgcache_assert_locked(pgcache); &(pgcache)->pc_queue; })

#define pgcache_btree_lock(pgcache)           ({btree_lock(pgcache_btree(pgcache)); })
#define pgcache_btree_unlock(pgcache)         ({btree_unlock(pgcache_btree(pgcache)); })
#define pgcache_btree_islocked(pgcache)       ({btree_islocked(pgcache_btree(pgcache)); })
#define pgcache_btree_assert_locked(pgcache)  ({btree_assert_locked(pgcache_btree(pgcache)); })

#define pgcache_queue_lock(pgcache)           ({ queue_lock(pgcache_queue(pgcache)); })
#define pgcache_queue_unlock(pgcache)         ({ queue_unlock(pgcache_queue(pgcache)); })
#define pgcache_queue_islocked(pgcache)       ({queue_islocked(pgcache_queue(pgcache)); })
#define pgcache_queue_assert_locked(pgcache)  ({queue_assert_locked(pgcache_queue(pgcache)); })

int pgcache_alloc(page_cache_t **ppcp);
void pgcache_free(page_cache_t *pgcache);
int pgcache_getpage(page_cache_t *pgcache, off_t pgno, page_t **page);
ssize_t pgcache_read(page_cache_t *pgcache, off_t off, void *buf, size_t nbyte);
ssize_t pgcache_write(page_cache_t *pgcache, off_t off, void *buf, size_t nbyte);