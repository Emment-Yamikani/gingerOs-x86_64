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

static int ptmx_open(struct devid *dd __unused, inode_t **pip) {
    int     err     = 0;
    PTY     pty     = NULL;
    cred_t  *cred   = NULL;
    inode_t *ip     = NULL; 

    if ((err = ptmx_alloc(&pty)))
        return err;

    if ((err = ialloc(FS_CHR, 0, &ip))) {
        ptmx_free(pty);
        return err;
    }

    cred = current_cred();

    cred_lock(cred);
    ip->i_mode= 0644;
    ip->i_gid = cred->c_gid;
    ip->i_uid = cred->c_uid;
    cred_unlock(cred);

    ip->i_size= 1;
    ip->i_priv=(void *)(u64)pty->pt_id;
    ip->i_rdev= DEV_T(DEV_PTMX, 2);

    pty->pt_imaster = ip;
    ip->i_priv      = (void *)pty;

    idupcnt(ip);

    if ((err = pts_create_slave(pty))) {
        idupcnt(ip);
        irelease(ip);
        ptmx_free(pty);
        return err;
    }

    pty_unlock(pty);
    *pip = ip;
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