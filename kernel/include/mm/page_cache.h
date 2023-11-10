#pragma once

#include <ds/btree.h>
#include <ds/queue.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <lib/types.h>
#include <sync/spinlock.h>

typedef struct icache_t icache_t;
typedef struct icache_t {
    long        pc_refcnt;
    uint32_t    pc_flags;
    inode_t     *pc_inode;
    btree_t     pc_btree;
    queue_t     pc_queue;
    size_t      pc_nrpages;
    spinlock_t  pc_lock;
} icache_t;

#define BTREE_INIT()    ((btree_t){0})
#define BTREE_NEW()     (&BTREE_INIT())

#define icache_assert(icache)               ({assert(icache, "No page cache"); })
#define icache_lock(icache)                 ({icache_assert(icache); spin_lock(&(icache)->pc_lock); })
#define icache_unlock(icache)               ({icache_assert(icache); spin_unlock(&(icache)->pc_lock); })
#define icache_islocked(icache)             ({icache_assert(icache); spin_islocked(&(icache)->pc_lock); })
#define icache_assert_locked(icache)        ({icache_assert(icache); spin_assert_locked(&(icache)->pc_lock); })

#define icache_btree(icache)                ({icache_assert_locked(icache); &(icache)->pc_btree; })
#define icache_queue(icache)                ({icache_assert_locked(icache); &(icache)->pc_queue; })

#define icache_btree_lock(icache)           ({btree_lock(icache_btree(icache)); })
#define icache_btree_unlock(icache)         ({btree_unlock(icache_btree(icache)); })
#define icache_btree_islocked(icache)       ({btree_islocked(icache_btree(icache)); })
#define icache_btree_assert_locked(icache)  ({btree_assert_locked(icache_btree(icache)); })

#define icache_queue_lock(icache)           ({ queue_lock(icache_queue(icache)); })
#define icache_queue_unlock(icache)         ({ queue_unlock(icache_queue(icache)); })
#define icache_queue_islocked(icache)       ({queue_islocked(icache_queue(icache)); })
#define icache_queue_assert_locked(icache)  ({queue_assert_locked(icache_queue(icache)); })

int icache_alloc(icache_t **ppcp);
void icache_free(icache_t *icache);
int icache_getpage(icache_t *icache, off_t pgno, page_t **page);
ssize_t icache_read(icache_t *icache, off_t off, void *buf, size_t size);
ssize_t icache_write(icache_t *icache, off_t off, void *buf, size_t size);