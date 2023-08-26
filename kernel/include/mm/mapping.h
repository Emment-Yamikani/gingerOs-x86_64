#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>
#include <lib/types.h>
#include <ds/btree.h>
#include <ds/queue.h>

typedef struct mapping
{
    uint32_t    m_flags;
    inode_t     *m_inode;
    btree_t     *m_btree;
    size_t      m_nrpages;
    queue_t     *m_usrmaps;
    spinlock_t  m_lock;
} mapping_t;

#define mapping_assert(mapping)         assert(mapping, "No mapping pointer")
#define mapping_lock(mapping)           ({mapping_assert(mapping); spin_lock(&(mapping)->m_lock);})
#define mapping_unlock(mapping)         ({mapping_assert(mapping); spin_unlock(&(mapping)->m_lock)})
#define mapping_holding(mapping)        ({mapping_assert(mapping); spin_holding(&(mapping)->m_lock);})
#define mapping_assert_locked(mapping)  ({mapping_assert(mapping); spin_assert_locked(&(mapping)->m_lock);})


int mapping_new(mapping_t **pmap);
void mapping_free(mapping_t *map);
int mapping_update_size(mapping_t *map, size_t size);
int mapping_free_page(mapping_t *map, ssize_t pgno);
int mapping_get_page(mapping_t *map, ssize_t pgno, uintptr_t *pphys, page_t **ppage);