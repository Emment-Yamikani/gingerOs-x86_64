#include <api.h>

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
static spinlock_t *liballoc_lk = SPINLOCK_NEW();


int liballoc_lock() {
	spin_lock(liballoc_lk);
	return 0;
}

int liballoc_unlock() {
	spin_unlock(liballoc_lk);
	return 0;
}

void* liballoc_alloc( int pages ) {
	if ( page_size < 0 ) page_size = getpagesize();
	unsigned int size = pages * page_size;
		
	char *p2 = (char*)mmap(0, size, PROT_NONE, MAP_PRIVATE|/*MAP_NORESERVE|*/MAP_ANONYMOUS, -1, 0);
	if ( p2 == MAP_FAILED) return NULL;

	if(mprotect(p2, size, PROT_READ|PROT_WRITE) != 0) {
		munmap(p2, size);
		return NULL;
	}

	return p2;
}

int liballoc_free( void* ptr, int pages ) {
	return munmap( ptr, pages * page_size );
}