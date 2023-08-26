#include <mm/mm_zone.h>
#include <mm/mapping.h>
#include <mm/page.h>
#include <bits/errno.h>
#include <mm/kalloc.h>
#include <lib/string.h>

int mapping_new(mapping_t **pmap) {
    int err = 0;
    btree_t *bt = NULL;
    mapping_t *map = NULL;
    queue_t *queue = NULL;

    if (pmap == NULL)
        return -EINVAL;
    
    if ((err = queue_new("address-map", &queue)))
        return err;

    if ((err = btree_alloc(&bt)))
        goto error;

    err = -ENOMEM;
    if (!(map = kmalloc(sizeof *map)))
        goto error;
    
    memset(map, 0, sizeof *map);

    map->m_btree = bt;
    map->m_usrmaps = queue;
    map->m_lock = SPINLOCK_INIT();
    mapping_lock(map);
    *pmap = map;

    return 0;
error:
    if (bt)
        btree_free(bt);

    if (queue)
        queue_free(queue);
    return err;
}

void mapping_free(mapping_t *map) {
    if (map == NULL)
        panic("%s:%d: No map\n", __FILE__, __LINE__);

    if (map->m_btree)
        btree_free(map->m_btree);
    if (map->m_usrmaps)
        queue_free(map->m_usrmaps);

    *map = (mapping_t){0};
    kfree(map);
}

int mapping_free_page(mapping_t *map, ssize_t pgno) {
    int err = 0;
    return 0;
error:
    return err;
}

int mapping_get_page(mapping_t *map, ssize_t pgno, uintptr_t *pphys, page_t **ppage) {
    int err = 0;
    return 0;
error:
    return err;
}