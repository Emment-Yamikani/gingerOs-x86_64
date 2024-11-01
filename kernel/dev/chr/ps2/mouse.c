#include <bits/errno.h>
#include <dev/dev.h>

static int      ps2mouse_init(void);
static int      ps2mouse_probe(void);
static int      ps2mouse_close(struct devid *dd);
static int      ps2mouse_getinfo(struct devid *dd, void *info);
static int      ps2mouse_open(struct devid *dd);
static int      ps2mouse_ioctl(struct devid *dd, int req, void *argp);
static off_t    ps2mouse_lseek(struct devid *dd, off_t off, int whence);
static ssize_t  ps2mouse_read(struct devid *dd, off_t off, void *buf, size_t sz);
static ssize_t  ps2mouse_write(struct devid *dd, off_t off, void *buf, size_t sz);
static int      ps2mouse_mmap(struct devid *dd, vmr_t *region);

static DEV_INIT(ps2mouse, FS_CHR, DEV_MOUSE0, 1);

static int ps2mouse_init(void) {
    printk("Initializing \e[025453;011m%s\e[0m chardev...\n", ps2mousedev.devname);
    return kdev_register(&ps2mousedev, DEV_MOUSE0, FS_CHR);
}

static int ps2mouse_probe(void) {
    return 0;
}

static int ps2mouse_close(struct devid *dd __unused) {
    return 0;
}

static int ps2mouse_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOTSUP;
}

static int ps2mouse_open(struct devid *dd __unused) {
    return 0;
}

static int ps2mouse_ioctl(struct devid *dd __unused, int req __unused, void *argp __unused) {
    return -ENOTSUP;
}

static off_t ps2mouse_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused) {
    return -ENOTSUP;
}

static ssize_t ps2mouse_read(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz __unused) {
    return 0;
}

static ssize_t ps2mouse_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz) {
    return sz;
}

static int ps2mouse_mmap(struct devid *dd, vmr_t *region) {
    if (dd == NULL || region == NULL)
        return -EINVAL;
    return -ENOSYS;
}

MODULE_INIT(ps2mouse, NULL, ps2mouse_init, NULL);