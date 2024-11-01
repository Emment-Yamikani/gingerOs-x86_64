#include <bits/errno.h>
#include <dev/dev.h>
#include <dev/pty.h>

static int      ptmx_init(void);
static int      ptmx_probe(void);
static int      ptmx_close(struct devid *dd);
static int      ptmx_mmap(struct devid *dd, vmr_t *region);
static int      ptmx_getinfo(struct devid *dd, void *info);
static int      ptmx_open(struct devid *dd, int oflags, ...);
static int      ptmx_ioctl(struct devid *dd, int req, void *argp);
static off_t    ptmx_lseek(struct devid *dd, off_t off, int whence);
static ssize_t  ptmx_read(struct devid *dd, off_t off, void *buf, size_t sz);
static ssize_t  ptmx_write(struct devid *dd, off_t off, void *buf, size_t sz);

static DEV_INIT(ptmx, FS_CHR, DEV_PTMX, 2);

static int ptmx_init(void) {
    int err = 0;

    printk("Initializing \e[025453;011m%s\e[0m chardev...\n", ptmxdev.devname);
    if ((err = pseudo_init()))
        return err;
    return kdev_register(&ptmxdev, DEV_PTMX, FS_CHR);
}

static int ptmx_probe(void) {
    return 0;
}

static int ptmx_close(struct devid *dd __unused) {
    return 0;
}

static int ptmx_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOTSUP;
}

static int ptmx_open(struct devid *dd __unused, int oflags __unused, ...) {
    return 0;
}

static int ptmx_ioctl(struct devid *dd __unused, int req __unused, void *argp __unused) {
    return -ENOTSUP;
}

static off_t ptmx_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused) {
    return -ENOTSUP;
}

static ssize_t ptmx_read(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz __unused) {
    return 0;
}

static ssize_t ptmx_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz) {
    return sz;
}

static int ptmx_mmap(struct devid *dd, vmr_t *region) {
    if (dd == NULL || region == NULL)
        return -EINVAL;
    return -ENOSYS;
}

MODULE_INIT(ptmx, NULL, ptmx_init, NULL);