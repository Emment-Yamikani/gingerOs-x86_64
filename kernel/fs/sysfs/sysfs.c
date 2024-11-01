#include <bits/errno.h>
#include <fs/fs.h>
#include <fs/sysfs.h>
#include <fs/tmpfs.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <mm/kalloc.h>

static filesystem_t *sysfs = NULL;

static iops_t sysfs_iops = {
    .iopen      = sysfs_iopen,
    .isync      = sysfs_isync,
    .iclose     = sysfs_iclose,
    .iunlink    = sysfs_iunlink,
    .itruncate  = sysfs_itruncate,
    .igetattr   = sysfs_igetattr,
    .isetattr   = sysfs_isetattr,
    .ifcntl     = sysfs_ifcntl,
    .iioctl     = sysfs_iioctl,
    .iread      = sysfs_iread_data,
    .iwrite     = sysfs_iwrite_data,
    .imkdir     = sysfs_imkdir,
    .icreate    = sysfs_icreate,
    .ibind      = sysfs_ibind,
    .ilookup    = sysfs_ilookup,
    .isymlink   = sysfs_isymlink,
    .imknod     = sysfs_imknod,
    .ireaddir   = sysfs_ireaddir,
    .ilink      = sysfs_ilink,
    .irename    = sysfs_irename,
};

static int sysfs_fill_sb(filesystem_t *fs __unused, const char *target,
                         struct devid *devid __unused, superblock_t *sb) {
    int         err    = 0;
    inode_t     *iroot = NULL;
    dentry_t    *droot = NULL;

    if ((err = tmpfs_new_inode(FS_DIR, &iroot)))
        return err;

    if ((err = dalloc(target, &droot))) {
        irelease(iroot);
        return err;
    }

    if ((err = iadd_alias(iroot, droot))) {
        dclose(droot);
        irelease(iroot);
        return err;
    }

    sb->sb_blocksize = -1;
    strncpy(sb->sb_magic0, "sysfs", 10);
    sb->sb_size     = -1;
    sb->sb_root     = droot;
    sb->sb_uio      = (cred_t){
        .c_gid      = 0,
        .c_uid      = 0,
        .c_umask    = 0665,
        .c_lock     = SPINLOCK_INIT(),
    };

    iroot->i_sb     = sb;
    iroot->i_type   = FS_DIR;
    iroot->i_ops    = sb->sb_iops;

    dunlock(droot);
    iunlock(iroot);
    return 0;
}

static int sysfs_getsb(filesystem_t *fs, const char *src __unused, const char *target,
                       unsigned long flags, void *data, superblock_t **psbp) {
    return getsb_nodev(fs, target, flags, data, psbp, sysfs_fill_sb);
}

int sysfs_init(void) {
    int err = 0;

    if ((err = fs_create("sysfs", &sysfs_iops, &sysfs)))
        return err;

    sysfs->get_sb = sysfs_getsb;
    sysfs->mount = NULL;

    if ((err = vfs_register_fs(sysfs))) {
        fsunlock(sysfs);
        goto error;
    }

    fsunlock(sysfs);
    return 0;
error:
    if (sysfs)
        fs_free(sysfs);
    return err;
}

int sysfs_iopen(inode_t *ip __unused) {
    return 0;
}

int sysfs_isync(inode_t *ip __unused) {
    return -ENOSYS;
}

int sysfs_iclose(inode_t *ip __unused) {
    return -ENOSYS;
}

int sysfs_iunlink(inode_t *ip __unused) {
    return -ENOSYS;
}

int sysfs_itruncate(inode_t *ip __unused) {
    return -ENOSYS;
}

int sysfs_igetattr(inode_t *ip __unused, void *attr __unused) {
    return -ENOSYS;
}

int sysfs_isetattr(inode_t *ip __unused, void *attr __unused) {
    return -ENOSYS;
}

int sysfs_ifcntl(inode_t *ip __unused, int cmd __unused, __unused void *argp __unused) {
    return -ENOSYS;
}

int sysfs_iioctl(inode_t *ip __unused, int req __unused, void *argp __unused) {
    return -ENOSYS;
}

ssize_t sysfs_iread_data(inode_t *ip __unused, off_t off __unused, void *buf __unused, size_t nb __unused) {
    return -ENOSYS;
}

ssize_t sysfs_iwrite_data(inode_t *ip __unused, off_t off __unused, void *buf __unused, size_t nb __unused) {
    return -ENOSYS;
}

int sysfs_imkdir(inode_t *dir __unused, const char *fname __unused, mode_t mode __unused) {
    return -ENOSYS;
}

int sysfs_icreate(inode_t *dir __unused, const char *fname __unused, mode_t mode __unused) {
    return -ENOSYS;
}

int sysfs_ibind(inode_t *dir __unused, struct dentry *dentry __unused, inode_t *ip __unused) {
    return -ENOSYS;
}

int sysfs_ilookup(inode_t *dir __unused, const char *fname __unused, inode_t **pipp __unused) {
    return -ENOSYS;
}

int sysfs_isymlink(inode_t *ip __unused, inode_t *atdir __unused, const char *symname __unused) {
    return -ENOSYS;
}

int sysfs_imknod(inode_t *dir __unused, const char *name __unused, mode_t mode __unused, int devid __unused) {
    return -ENOSYS;
}

ssize_t sysfs_ireaddir(inode_t *dir __unused, off_t off __unused, struct dirent *buf __unused, size_t count __unused) {
    return -ENOSYS;
}

int sysfs_ilink(const char *oldname __unused, inode_t *dir __unused, const char *newname __unused) {
    return -ENOSYS;
}

int sysfs_irename(inode_t *dir __unused, const char *old __unused, inode_t *newdir __unused, const char *new __unused) {
    return -ENOSYS;
}