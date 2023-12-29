#include <mm/mm_zone.h>
#include <fs/icache.h>
#include <mm/page.h>
#include <arch/paging.h>
#include <bits/errno.h>
#include <mm/kalloc.h>
#include <lib/string.h>
#include <fs/inode.h>
#include <lib/stddef.h>

int icache_alloc(icache_t **ppcp) {
    icache_t *icache = NULL;

    if (ppcp == NULL)
        return -EINVAL;

    if ((icache = kmalloc(sizeof *icache)) == NULL)
        return -ENOMEM;
    
    memset(icache, 0, sizeof *icache);

    icache->pc_flags    = 0;
    icache->pc_refcnt   = 1;
    icache->pc_nrpages  = 0;
    icache->pc_btree    = BTREE_INIT();
    icache->pc_queue    = QUEUE_INIT();
    icache->pc_lock     = SPINLOCK_INIT();

    *ppcp               = icache;
    return 0;
}

void icache_free(icache_t *icache) {
    int unlock = 0;
    
    if (icache == NULL)
        return;
    
    if ((unlock = !icache_islocked(icache)))
        icache_lock(icache);
    
    --icache->pc_refcnt;

    if (icache->pc_refcnt <= 0) {
        icache_btree_lock(icache);
        btree_flush(icache_btree(icache));
        icache_btree_unlock(icache);

        icache_queue_lock(icache);
        queue_flush(icache_queue(icache));
        icache_queue_unlock(icache);

        icache_unlock(icache);
        kfree(icache);
    } else {
        if (unlock != 0)
            icache_unlock(icache);
    }
}

int icache_getpage(icache_t *icache, off_t pgno, page_t **ref) {
    int     err         = 0;
    ssize_t size        = 0;
    int     new_page    = 0;
    page_t *page        = NULL;
    char    buff[PGSZ]  = {0};

    if (ref == NULL || icache == NULL)
        return -EINVAL;    

    icache_assert_locked(icache);
    
    icache_btree_lock(icache);
    if (0 == (err = btree_search(icache_btree(icache), pgno, (void **)&page))) {
        icache_btree_unlock(icache);
        if (!page_isvalid(page))
            goto update;
        goto done;
    }
    icache_btree_unlock(icache);

    err = -ENOMEM;
    if ((page = alloc_page(GFP_KERNEL | GFP_ZERO)) == NULL)
        goto error;
    
    new_page = 1;

update:
    if ((err = (size = iread_data(icache->pc_inode, pgno * PGSZ, buff, PGSZ))) < 0)
        goto error;

    if ((err = arch_memcpyvp(page_address(page), (uintptr_t)buff, size)))
        goto error;
    
    page_setvalid(page);
    page_maskdirty(page);

    if (new_page) {
        icache_btree_lock(icache);
        if ((err = btree_insert(icache_btree(icache), pgno, page))) {
            icache_btree_unlock(icache);
            goto error;
        }
        icache_btree_unlock(icache);
    }
done:
    *ref = page;
    return 0;
error:
    if (new_page)
        page_put(page);
    return err;
}

ssize_t icache_read(icache_t *icache, off_t off, void *buff, size_t sz) {
    ssize_t err         = 0;
    off_t   pgno        = 0;    /*page number*/
    size_t  size        = 0;    /*size to read*/
    ssize_t  total      = 0;    /*total bytes read*/
    size_t  pagesz      = 0;
    off_t   offset      = off;  /*current offset*/
    page_t *page        = NULL;

    icache_assert_locked(icache);
    for (; sz; sz -= size, total += size, offset += size) {
        pgno = offset / PGSZ;
        if ((err = (ssize_t)icache_getpage(icache, pgno, &page)))
            goto error;

        pagesz = PAGESZ - (offset % PGSZ);
        size = MIN(pagesz, sz);
        if ((err = (ssize_t)arch_memcpypv((uintptr_t)buff + total,
                      page_address(page) + (offset % PGSZ), size)))
            goto error;
    }

    return total;
error:
    return err;
}

ssize_t icache_write(icache_t *icache, off_t off, void *buff, size_t sz) {
    ssize_t     err     = 0;
    off_t       pgno    = 0;    /*page number*/
    ssize_t     total   = 0;    /*total bytes written*/
    size_t      size    = 0;    /*size to write*/
    size_t      pagesz  = 0;
    off_t       offset  = off;  /*current offset*/
    page_t      *page   = NULL;

    icache_assert_locked(icache);
    for (; sz; sz -= size, total += size, offset += size) {
        pgno = offset / PGSZ;
        if ((err = (ssize_t)icache_getpage(icache, pgno, &page)))
            goto error;

        pagesz = PAGESZ - (offset % PGSZ);
        size = MIN(pagesz, sz);
        if ((err = (ssize_t)arch_memcpyvp(page_address(page) + (offset % PGSZ),
                                 (uintptr_t)buff + total, size)))
            goto error;

        page_setdirty(page);
    }

    return total;
error:
    return err;
}