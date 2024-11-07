#include <bits/errno.h>
#include <dev/dev.h>
#include <dev/pty.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <sys/thread.h>
#include <mm/kalloc.h>

DEV_DECL_OPS(static, pts);

int pts_create_slave(PTY pty) {
    char    name[32];
    int     err     = 0;
    dev_t   *dev    = NULL;
    mode_t  mode    = S_IFCHR | 0; // mode will be set by grantpt().

    if (pty == NULL)
        return -EINVAL;

    if ((err = kdev_create(name, FS_CHR, DEV_PTS, pty->pt_id, &dev)))
        return err;

    dev->devops = DEVOPS(pts);
    dev->devprobe = pts_probe;

    if ((err = kdev_register(dev, DEV_PTS, FS_CHR))) {
        dev_unlock(dev);
        kfree(dev);
        return err;
    }

    dev_unlock(dev);

    memset(name, 0, sizeof name);
    snprintf(name, sizeof name - 1, "/dev/pts/%d", pty->pt_id);

    if ((err = vfs_mknod(name, current_cred(), mode, DEV_T(DEV_PTS, pty->pt_id)))) {
        assert_msg(0, "%s:%d: error mknod: pts, err: %d\n",
            __FILE__, __LINE__, err);
        dev_unlock(dev);
        kfree(dev);
        return err;
    }

    return 0;
}

__unused static int pts_init(void) {
    return 0;
}

static int pts_probe(void) {
    return 0;
}

static int pts_close(struct devid *dd __unused) {
    return 0;
}

static int pts_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOTSUP;
}

static int pts_open(struct devid *dd __unused, inode_t **pip __unused) {
    return 0;
}

static int pts_ioctl(struct devid *dd __unused, int req __unused, void *argp __unused) {
    return -ENOTSUP;
}

static off_t pts_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused) {
    return -ENOTSUP;
}

static ssize_t pts_read(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz __unused) {
    return 0;
}

static ssize_t pts_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz) {
    return sz;
}

static int pts_mmap(struct devid *dd, vmr_t *region) {
    if (dd == NULL || region == NULL)
        return -EINVAL;
    return -ENOSYS;
}