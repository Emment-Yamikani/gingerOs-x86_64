#include <lib/printk.h>
#include <mm/vmm.h>
#include <lib/stdlib.h>
#include <sync/spinlock.h>
#include <arch/paging.h>


#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
# define MAP_ANONYMOUS MAP_ANON
#endif
#if !defined(MAP_FAILED)
# define MAP_FAILED ((char*)-1)
#endif

#ifndef MAP_NORESERVE
# ifdef MAP_AUTORESRV
#  define MAP_NORESERVE MAP_AUTORESRV
# else
#  define MAP_NORESERVE 0
# endif
#endif

static int page_size = -1;
static spinlock_t *spinlock = &SPINLOCK_INIT();


int liballoc_lock() {
	spin_lock(spinlock);
	return 0;
}

int liballoc_unlock() {
	spin_unlock(spinlock);
	return 0;
}

void* liballoc_alloc( int pages ) {
	char *p2 = NULL;
	if ( page_size < 0 ) page_size = getpagesize();
	size_t size = pages * page_size;
	// printk("\nliballoc_alloc(): pointer: request pages: %d\n", pages);
	if (arch_pagealloc(size, (uintptr_t *)&p2))
		return NULL;
	// printk("liballoc_alloc(): pointer: %p request pages: %d\n", p2, pages);
	return p2;
}

int liballoc_free( void* ptr, int pages ) {
	// printk("liballoc_free(): release pointer: %p, pages: %d\n", ptr, pages);
	arch_pagefree((uintptr_t)ptr, pages * page_size);
	return 0;
}