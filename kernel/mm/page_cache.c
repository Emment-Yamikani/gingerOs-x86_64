#include <mm/mm_zone.h>
#include <mm/page_cache.h>
#include <mm/page.h>
#include <arch/paging.h>
#include <bits/errno.h>
#include <mm/kalloc.h>
#include <lib/string.h>
#include <fs/inode.h>

int pgcache_alloc(page_cache_t **ppcp) {
    page_cache_t *pgcache = NULL;

    if (ppcp == NULL)
        return -EINVAL;

    if ((pgcache = kmalloc(sizeof *pgcache)) == NULL)
        return -ENOMEM;
    
    memset(pgcache, 0, sizeof *pgcache);

    pgcache->pc_flags = 0;
    pgcache->pc_refcnt = 1;
    pgcache->pc_nrpages = 0;
    pgcache->pc_btree = BTREE_INIT();
    pgcache->pc_queue = QUEUE_INIT();
    pgcache->pc_lock = SPINLOCK_INIT();
    *ppcp = pgcache;

    return 0;
}

void pgcache_free(page_cache_t *pgcache) {
    int unlock = 0;
    
    if (pgcache == NULL)
        return;
    
    if ((unlock = !pgcache_islocked(pgcache)))
        pgcache_lock(pgcache);
    
    --pgcache->pc_refcnt;

    if (pgcache->pc_refcnt <= 0) {
        pgcache_btree_lock(pgcache);
        btree_flush(pgcache_btree(pgcache));
        pgcache_btree_unlock(pgcache);

        pgcache_queue_lock(pgcache);
        queue_flush(pgcache_queue(pgcache));
        pgcache_queue_unlock(pgcache);

        pgcache_unlock(pgcache);
        kfree(pgcache);
    } else {
        if (unlock != 0)
            pgcache_unlock(pgcache);
    }
}

int pgcache_getpage(page_cache_t *pgcache, off_t pgno __unused, page_t **ppgp) {
    int err = 0;
    page_t *pg = NULL;

    pgcache_assert_locked(pgcache);

    if (pgcache == NULL || ppgp == NULL)
        return -EINVAL;

    if ((pg = alloc_page(GFP_KERNEL)) == NULL)
        return -ENOMEM;
    
    


    return 0;
error: __unused
    return err;
}

ssize_t pgcache_read(page_cache_t *pgcache, off_t off, void *buf, size_t nbyte);

ssize_t pgcache_write(page_cache_t *pgcache, off_t off, void *buf, size_t nbyte);