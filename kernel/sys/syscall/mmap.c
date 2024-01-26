#include <fs/fs.h>
#include <arch/cpu.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <lib/printk.h>
#include <mm/mmap.h>
#include <bits/errno.h>
#include <sys/sysproc.h>
#include <arch/paging.h>

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off) {
    long            err = 0;
    mmap_t          *mmap   = NULL;
    file_t          *file   = NULL;
    vmr_t           *region = NULL;

    printk("[%d:%d:%d]: mmap(%p, %d, %d, %d, %d, %d)\n",
        thread_self(), getpid(), getppid(), addr, len, prot, flags, fd, off);

    // MAP_ANON can only be passed with fd == -1, return error otherwise.
    if (__flags_anon(flags) && fd != -1)
        return (void *)-EINVAL;

    // get the address space(mmap) struct of the current process.
    mmap = curproc->mmap;
    mmap_lock(mmap);

    if ((err = mmap_map_region(mmap, (uintptr_t)addr, len, prot, flags, &region)))
        goto error;

    addr = (void *)__vmr_start(region);

    if (__flags_anon(flags))
        goto anon;

    /**
     * @brief Get here, the fildes (file_t *) based on the argument 'fd' passed to us.
     * fildes shall be used to make a driver specific mmap() call to map the memory
     * region to fd.
    */
    if ((err = file_get(fd, &file)))
        goto error;
    
    region->filesz  = len;
    region->file_pos= off;
    region->flags  |= VM_FILE;
    region->memsz   = __vmr_size(region);

    if ((err = fmmap(file, region))) {
        goto error;
        funlock(file);
    }

    funlock(file);

    goto done;
    // make an annonymous mapping.
anon:
    // if (__flags_mapin(flags)) {
    //     if ((err = paging_mappages(PGROUND(addr),
    //         ALIGN4KUP(__vmr_size(region)), region->vflags)))
    //         goto error;
    //     if (__flags_zero(flags))
    //         memset((void *)PGROUND(addr), 0,
    //             ALIGN4KUP(__vmr_size(region)));
    // }

    // mmap() operation is done.
done:
    mmap_unlock(mmap);
    return addr;
error:
    if (mmap && region)
        mmap_remove(mmap, region);

    if (mmap)
        mmap_dump_list(*mmap);

    if (mmap && mmap_islocked(mmap))
        mmap_unlock(mmap);

    printk("err: %d\n", err);
    return (void *)err;
}

int munmap(void *addr, size_t len) {
    int err = -EINVAL;
    mmap_t *mmap = NULL;
    vmr_t *region = NULL;

    mmap = curproc->mmap;

    if (mmap == NULL)
        return -EINVAL;

    if (!__isaligned((uintptr_t)addr))
        return -EINVAL;

    mmap_lock(mmap);

    if ((region = mmap_find(mmap, (uintptr_t)addr)) == NULL)
        goto error;

    mmap_unmap(mmap, (uintptr_t)addr, PGROUND(len));
    mmap_unlock(mmap);

    return 0;
error:
    mmap_unlock(mmap);
    return err;
}

int mprotect(void *addr, size_t len, int prot) {
    mmap_t *mmap = NULL;
    mmap = curproc->mmap;

    if (mmap == NULL)
        return -EINVAL;

    mmap_lock(mmap);
    int err = mmap_protect(mmap, (uintptr_t)addr, len, prot);
    mmap_unlock(mmap);
    return err;
}