#include <arch/paging.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <mm/zone.h>
#include <sys/thread.h>

static page_t *address_get_page(uintptr_t addr) {
    int    err   = 0;
    page_t *page = NULL;
    zone_t *zone = NULL;

     assert(!(err = getzone_byaddr(addr, PGSZ, &zone)),
        "Couldn't get zone by page(%p). error: %d\n", addr, err
    );

    page = &zone->pages[(addr - zone->start) / PGSZ];
    if (!atomic_read(&page->refcnt) ||
            !bitmap_test(&zone->bitmap, page - zone->pages, 1)) {
        assert(0, "Page(%p): couldn't take a reference to the page handle.\n", addr);
    }

    zone_unlock(zone);
    return page;
}

void page_free_n(page_t *page, usize order) {
    int     err     = 0;
    usize   pos     = 0;
    zone_t  *zone   = NULL;
    usize   npage   = BS(order);

    page_assert(page);

    assert(order <= 64, "Requested order(%d) is greater than 64.\n", order);

    assert(!(err = getzone_bypage(page, &zone)),
        "Couldn't get zone by page(%p). error: %d\n", page, err
    );

    zone_assert_isnotkernel(zone, page);

    if (page_end(page, npage, zone) >= zone_end(zone)) {
        printk("%s@%s:%d: [Warning] Out of ZONE deallocation.\nNo page was deallocated.",
            __func__, __FILE__, __LINE__);
        zone_unlock(zone);
        return;
    }

    for (pos = page - zone->pages; npage != 0; --npage, ++page, ++pos) {
        assert(atomic_read(&page->refcnt),
            "Double free detected for page(%p).\n", page_addr(page, zone)
        );

        /// page still has some references to it.
        /// so continue freeing other pages, if any.
        if (atomic_dec_fetch(&page->refcnt))
            continue;

        if ((err = bitmap_unset(&zone->bitmap, pos, 1))) {
            assert(err == 0, "Bitmap free failed. error: %d\n", err);
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

void __page_free_n(uintptr_t addr, usize order) {
    page_t  *page   = NULL;

    assert(addr, "Invalid physical address.");
    page = address_get_page(addr);
    page_free_n(page, order);
}

void __page_free(uintptr_t addr) {
    __page_free_n(addr, 0);
}