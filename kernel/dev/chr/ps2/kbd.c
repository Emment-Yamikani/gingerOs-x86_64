#include <bits/errno.h>
#include <dev/dev.h>

DEV_DECL_OPS(static, ps2kbd);

static DEV_INIT(ps2kbd, FS_CHR, DEV_KBD0, 0);

static int ps2kbd_init(void) {
    printk("Initializing \e[025453;011m%s\e[0m chardev...\n", ps2kbddev.devname);
    return kdev_register(&ps2kbddev, DEV_KBD0, FS_CHR);
}

static int ps2kbd_probe(void) {
    return 0;
}

static int ps2kbd_close(struct devid *dd __unused) {
    return 0;
}

static int ps2kbd_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOTSUP;
}

static int ps2kbd_open(struct devid *dd __unused, inode_t **pip __unused) {
    return 0;
}

static int ps2kbd_ioctl(struct devid *dd __unused, int req __unused, void *argp __unused) {
    return -ENOTSUP;
}

static off_t ps2kbd_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused) {
    return -ENOTSUP;
}

static ssize_t ps2kbd_read(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz __unused) {
    return 0;
}

static ssize_t ps2kbd_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz) {
    return sz;
}

static int ps2kbd_mmap(struct devid *dd, vmr_t *region) {
    if (dd == NULL || region == NULL)
        return -EINVAL;
    return -ENOSYS;
}

MODULE_INIT(ps2kbd, NULL, ps2kbd_init, 0ULL);