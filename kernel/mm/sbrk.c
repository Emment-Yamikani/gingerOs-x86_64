#include <mm/mmap.h>
#include <bits/errno.h>
#include <sys/thread.h>

void *sbrk(intptr_t increment) {
    long        err     = 0;
    uintptr_t   brk     = 0;

    if (current == NULL || current->t_mmap == NULL)
        return (void *)-1;


    mmap_lock(current->t_mmap);

    if (current->t_mmap->heap == NULL) {
        if ((err = mmap_alloc_vmr(current->t_mmap, UHEAP_MAX,
            PROT_RWX, MAP_PRIVATE, &current->t_mmap->heap))) {
            mmap_unlock(current->t_mmap);
            return (void *)err;
        }
        
        current->t_mmap->brk = __vmr_start(current->t_mmap->heap);
    }
    
    if ((err = mmap_vmr_expand(current->t_mmap, current->t_mmap->heap, increment))) {
        mmap_unlock(current->t_mmap);
        return (void *)err;
    }

    current->t_mmap->brk = (isize)(brk = current->t_mmap->brk) + increment;

    mmap_unlock(current->t_mmap);
    return (void *)brk;
}