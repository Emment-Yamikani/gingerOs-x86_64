#include <dev/dev.h>
#include <lib/string.h>
#include <sync/spinlock.h>
#include <lib/printk.h>
#include <bits/errno.h>

DEV_DECL_OPS(static, random);
static DEV_INIT(random, FS_CHR, DEV_RANDOM, 8);

static int random_init(void) {
    printk("Initializing \e[025453;011m%s\e[0m chardev...\n", randomdev.dev_name);
    return kdev_register(&randomdev, DEV_RANDOM, FS_CHR);
}

static int random_probe(void) {
    return 0;
}

static int random_close(struct devid *dd __unused) {
    return 0;
}

static int random_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOTSUP;
}

static int random_open(struct devid *dd __unused, inode_t **pip __unused) {
    return 0;
}

static int random_ioctl(struct devid *dd __unused, int req __unused, void *argp __unused)
{
    return -ENOTSUP;
}

static off_t random_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused) {
    return -ENOTSUP;
}

static ssize_t random_read(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz __unused) {
    return -ENOTSUP;
}

static ssize_t random_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz __unused) {
    return -ENOTSUP;
}

static int random_mmap(struct devid *dd, vmr_t *region) {
    if (dd == NULL || region == NULL)
        return -EINVAL;
    
    return -ENOSYS;
}

MODULE_INIT(random, NULL, random_init, NULL);