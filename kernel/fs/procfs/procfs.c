#include <bits/errno.h>
#include <fs/fs.h>
#include <fs/procfs.h>
#include <fs/tmpfs.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <mm/kalloc.h>

static filesystem_t *procfs = NULL;

static iops_t procfs_iops = {
    .isync      = procfs_isync,
    .iclose     = procfs_iclose,
    .iunlink    = procfs_iunlink,
    .itruncate  = procfs_itruncate,
    .igetattr   = procfs_igetattr,
    .isetattr   = procfs_isetattr,
    .ifcntl     = procfs_ifcntl,
    .iioctl     = procfs_iioctl,
    .iread      = procfs_iread_data,
    .iwrite     = procfs_iwrite_data,
    .imkdir     = procfs_imkdir,
    .icreate    = procfs_icreate,
    .ibind      = procfs_ibind,
    .ilookup    = procfs_ilookup,
    .isymlink   = procfs_isymlink,
    .imknod     = procfs_imknod,
    .ireaddir   = procfs_ireaddir,
    .ilink      = procfs_ilink,
    .irename    = procfs_irename,
};

static int procfs_fill_sb(filesystem_t *fs __unused, const char *target,
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
    strncpy(sb->sb_magic0, "procfs", 10);
    sb->sb_size     = -1;
    sb->sb_root     = droot;
    sb->sb_uio      = (cred_t){
        .c_gid      = 0,
        .c_uid      = 0,
        .c_umask    = 0555,
        .c_lock     = SPINLOCK_INIT(),
    };

    iroot->i_sb     = sb;
    iroot->i_type   = FS_DIR;
    iroot->i_ops    = sb->sb_iops;

    dunlock(droot);
    iunlock(iroot);
    return 0;
}

static int procfs_getsb(filesystem_t *fs, const char *src __unused, const char *target,
                       unsigned long flags, void *data, superblock_t **psbp) {
    return getsb_nodev(fs, target, flags, data, psbp, procfs_fill_sb);
}

int procfs_init(void) {
    int err = 0;

    if ((err = fs_create("procfs", &procfs_iops, &procfs)))
        return err;

    procfs->get_sb = procfs_getsb;
    procfs->mount = NULL;

    if ((err = vfs_register_fs(procfs))) {
        fsunlock(procfs);
        goto error;
    }

    fsunlock(procfs);
    return 0;
error:
    if (procfs)
        fs_free(procfs);
    return err;
}

int procfs_isync(inode_t *ip __unused) {
    return -ENOSYS;
}

int procfs_iclose(inode_t *ip __unused) {
    return -ENOSYS;
}

int procfs_iunlink(inode_t *ip __unused) {
    return -ENOSYS;
}

int procfs_itruncate(inode_t *ip __unused) {
    return -ENOSYS;
}

int procfs_igetattr(inode_t *ip __unused, void *attr __unused) {
    return -ENOSYS;
}

int procfs_isetattr(inode_t *ip __unused, void *attr __unused) {
    return -ENOSYS;
}

int procfs_ifcntl(inode_t *ip __unused, int cmd __unused, __unused void *argp __unused) {
    return -ENOSYS;
}

int procfs_iioctl(inode_t *ip __unused, int req __unused, void *argp __unused) {
    return -ENOSYS;
}

ssize_t procfs_iread_data(inode_t *ip __unused, off_t off __unused, void *buf __unused, size_t nb __unused) {
    return -ENOSYS;
}

ssize_t procfs_iwrite_data(inode_t *ip __unused, off_t off __unused, void *buf __unused, size_t nb __unused) {
    return -ENOSYS;
}

int procfs_imkdir(inode_t *dir __unused, const char *fname __unused, mode_t mode __unused) {
    return -ENOSYS;
}

int procfs_icreate(inode_t *dir __unused, const char *fname __unused, mode_t mode __unused) {
    return -ENOSYS;
}

int procfs_ibind(inode_t *dir __unused, struct dentry *dentry __unused, inode_t *ip __unused) {
    return -ENOSYS;
}

int procfs_ilookup(inode_t *dir __unused, const char *fname __unused, inode_t **pipp __unused) {
    return -ENOSYS;
}

int procfs_isymlink(inode_t *ip __unused, inode_t *atdir __unused, const char *symname __unused) {
    return -ENOSYS;
}

int procfs_imknod(inode_t *dir __unused, const char *name __unused, mode_t mode __unused, int devid __unused) {
    return -ENOSYS;
}

ssize_t procfs_ireaddir(inode_t *dir __unused, off_t off __unused, struct dirent *buf __unused, size_t count __unused) {
    return -ENOSYS;
}

int procfs_ilink(const char *oldname __unused, inode_t *dir __unused, const char *newname __unused) {
    return -ENOSYS;
}

int procfs_irename(inode_t *dir __unused, const char *old __unused, inode_t *newdir __unused, const char *new __unused) {
    return -ENOSYS;
}