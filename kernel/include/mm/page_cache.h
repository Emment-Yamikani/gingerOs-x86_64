#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>
#include <lib/types.h>
#include <ds/btree.h>
#include <ds/queue.h>

typedef struct page_cache_t page_cache_t;
typedef struct page_cache_t {
    uint32_t    m_flags;
    inode_t     *m_inode;
    btree_t     m_btree;
    queue_t     m_usrmaps;
    size_t      m_nrpages;
    spinlock_t  m_lock;
} page_cache_t;

