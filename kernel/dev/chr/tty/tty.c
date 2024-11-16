#include <bits/errno.h>
#include <dev/tty.h>
#include <lib/string.h>
#include <lib/printk.h>
#include <sys/thread.h>

DEV_DECL_OPS(static, tty);
static DEV_INIT(tty, FS_CHR, DEV_TTY, 0);

static int tty_probe(void) {
    return 0;
}

static int tty_init(void) {
    printk("Initializing \e[025453;011m%s\e[0m chardev...\n", ttydev.devname);
    return kdev_register(&ttydev, DEV_TTY, FS_CHR);
}

MODULE_INIT(tty, NULL, tty_init, NULL);

static int     tty_close(struct devid *dd __unused) {
    return 0;
}

static int     tty_open(struct devid *dd __unused, inode_t **pip __unused) {
    return 0;
}

static int     tty_mmap(struct devid *dd __unused, vmr_t *region __unused) {
    return -ENOSYS;
}

static int     tty_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOSYS;
}

static off_t   tty_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused __unused) {
    return -ENOSYS;
}

static int     tty_ioctl(struct devid *dd __unused, int request __unused, void *arg __unused __unused) {
    return -ENOSYS;
}

static ssize_t tty_read(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t nbyte __unused) {
    return -ENOSYS;
}

static ssize_t tty_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t nbyte __unused) {
    return -ENOSYS;
}