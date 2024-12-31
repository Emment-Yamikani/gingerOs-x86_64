#include <arch/paging.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <mm/zone.h>
#include <sys/thread.h>

void page_free_n(page_t *page, usize order) {
    int     err     = 0;
    usize   pos     = 0;
    zone_t  *zone   = NULL;
    usize   npage   = BS(order);

    page_assert(page);

    assert_msg(order <= 64, "%s@%s:%d: Requested order(%d) is greater than 64.\n",
        __func__, __FILE__, __LINE__, order
    );

    assert_msg(!(err = getzone_bypage(page, &zone)),
        "%s@%s:%d: [PANICED] Couldn't get zone by page(%p). error: %d\n",
        __func__, __FILE__, __LINE__, page, err
    );

    zone_assert_isnotkernel(zone, page);

    if (page_end(page, npage, zone) >= zone_end(zone)) {
        printk("%s@%s:%d: [Warning] Out of ZONE deallocation.\nNo page was deallocated.",
            __func__, __FILE__, __LINE__);
        zone_unlock(zone);
        return;
    }

    for (pos = page - zone->pages; npage != 0; --npage, ++page, ++pos) {
        assert_msg(atomic_read(&page->refcnt),
            "%s@%s:%d: Double free detected for page(%p).\n",
            __func__, __FILE__, __LINE__, page_addr(page, zone)
        );

        /// page still has some references to it.
        /// so continue freeing other pages, if any.
        if (atomic_dec_fetch(&page->refcnt))
            continue;

        if ((err = bitmap_unset(&zone->bitmap, pos, 1))) {
            assert_msg(err == 0, "%s@%s:%d: Bitmap free failed. error: %d\n",
                __func__, __FILE__, __LINE__, err
            );
        }

        page_resetflags(page);
        page_setswappable(page);

        zone->upages    -= 1;

        page->virtual   = 0;
        page->icache    = NULL;
    }
    

    zone_unlock(zone);
}

void page_free(page_t *page) {
    page_free_n(page, 0);
}

void __page_free_n(uintptr_t paddr, usize order) {
    int     err     = 0;
    usize   pos     = 0;
    page_t  *page   = NULL;
    zone_t  *zone   = NULL;
    usize   npage   = BS(order);

    assert(paddr, "Invalid physical address.");

    assert_msg(order <= 64, "%s@%s:%d: Requested order(%d) is greater than 64.\n",
        __func__, __FILE__, __LINE__, order
    );

    assert_msg(!(err = getzone_byaddr(paddr, npage * PGSZ, &zone)),
        "%s@%s:%d: [PANICED] Couldn't get zone by page(%p). error: %d\n",
        __func__, __FILE__, __LINE__, paddr, err
    );

    page = &zone->pages[(paddr - zone->start) / PGSZ];

    zone_assert_isnotkernel(zone, page);

    if (page_end(page, npage, zone) >= zone_end(zone)) {
        printk("%s@%s:%d: [Warning] Out of ZONE deallocation.\nNo page was deallocated.",
            __func__, __FILE__, __LINE__);
        zone_unlock(zone);
        return;
    }

    for (pos = page - zone->pages; npage != 0; --npage, ++page, ++pos) {
        assert_msg(atomic_read(&page->refcnt),
            "%s@%s:%d: Double free detected for page(%p).\n",
            __func__, __FILE__, __LINE__, page_addr(page, zone)
        );

        /// page still has some references to it.
        /// so continue freeing other pages, if any.
        if (atomic_dec_fetch(&page->refcnt))
            continue;

        if ((err = bitmap_unset(&zone->bitmap, pos, 1))) {
            assert_msg(err == 0, "%s@%s:%d: Bitmap free failed. error: %d\n",
                __func__, __FILE__, __LINE__, err
            );
        }

        page_resetflags(page);
        page_setswappable(page);

        zone->upages    -= 1;

        page->virtual   = 0;
        page->icache    = NULL;
    }
    

    zone_unlock(zone);
}

void __page_free(uintptr_t paddr) {
    __page_free_n(paddr, 0);
}
