#include <dev/dev.h>
#include <lib/string.h>
#include <sync/spinlock.h>
#include <lib/printk.h>
#include <bits/errno.h>

DEV_DECL_OPS(static, zero);

static DEV_INIT(zero, FS_CHR, DEV_ZERO, 5);

static int zero_init(void) {
    printk("Initializing \e[025453;011m%s\e[0m chardev...\n", zerodev.devname);
    return kdev_register(&zerodev, DEV_ZERO, FS_CHR);
}

static int zero_probe(void) {
    return 0;
}

static int zero_close(struct devid *dd __unused) {
    return 0;
}

static int zero_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOTSUP;
}

static int zero_open(struct devid *dd __unused, inode_t **pip __unused) {
    return 0;
}

static int zero_ioctl(struct devid *dd __unused, int req __unused, void *argp __unused) {
    return -ENOTSUP;
}

static off_t zero_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused) {
    return -ENOTSUP;
}

static ssize_t zero_read(struct devid *dd __unused, off_t off __unused, void *buf, size_t sz) {
    memset(buf, '\0', sz);
    return sz;
}

static ssize_t zero_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz) {
    return sz;
}

static int zero_mmap(struct devid *dd, vmr_t *region) {
    if (dd == NULL || region == NULL)
        return -EINVAL;
    
    return -ENOSYS;
}

MODULE_INIT(zero, NULL, zero_init, NULL);