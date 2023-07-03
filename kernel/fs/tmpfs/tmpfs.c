#include <bits/errno.h>
#include <fs/generic_tmpfs.h>

static iops_t tmpfs_iops = {0};
static filesystem_t *tmpfs = NULL;

int tmpfs_init(void) {
    int err = 0;
    tmpfs_iops = generic_tmpfs_iops;

    if ((err = vfs_filesystem_register("tmpfs", *__UIO("/"), 0, &tmpfs_iops, &tmpfs)))
        goto error;
    
    return 0;
error:
    return err;
}