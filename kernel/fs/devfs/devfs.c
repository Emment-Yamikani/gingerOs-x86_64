#include <fs/generic_tmpfs.h>

static iops_t devfs_iops = {0};

int devfs_init(void) {
    devfs_iops = generic_tmpfs_iops;
    return 0;
}