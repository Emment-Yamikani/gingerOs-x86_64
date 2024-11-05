#include <bits/errno.h>
#include <dev/dev.h>
#include <dev/pty.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <sys/thread.h>

DEV_DECL_OPS(static, ptmx);

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

static int ptmx_open(struct devid *dd __unused, inode_t **pip __unused) {
    char    name[64];
    int     err     = 0;
    PTY     pty     = NULL;
    mode_t  mode    = S_IFCHR | 0; // mode will be set by grantpt().
    cred_t  *cred   = NULL;

    if ((err = ptmx_alloc(&pty)))
        return err;

    memset(name, 0, sizeof name);
    snprintf(name, sizeof name, "/dev/pts/%d", pty->pt_id);

    cred = current_cred();

    if ((err = vfs_mknod(name, cred, mode, DEV_T(DEV_PTS, pty->pt_id)))) {
        printk("%s:%d: Failed, error: %d\n", __FILE__, __LINE__, err);
        goto error;
    }


    return 0;
error:
    return err;
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