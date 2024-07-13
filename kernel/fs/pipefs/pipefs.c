#include <bits/errno.h>
#include <fs/fs.h>
#include <fs/pipefs.h>
#include <fs/tmpfs.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <mm/kalloc.h>

static filesystem_t *pipefs = NULL;

static iops_t pipefs_iops = {
    .isync      = pipefs_isync,
    .iclose     = pipefs_iclose,
    .iunlink    = pipefs_iunlink,
    .itruncate  = pipefs_itruncate,
    .igetattr   = pipefs_igetattr,
    .isetattr   = pipefs_isetattr,
    .ifcntl     = pipefs_ifcntl,
    .iioctl     = pipefs_iioctl,
    .iread      = pipefs_iread_data,
    .iwrite     = pipefs_iwrite_data,
    .imkdir     = pipefs_imkdir,
    .icreate    = pipefs_icreate,
    .ibind      = pipefs_ibind,
    .ilookup    = pipefs_ilookup,
    .isymlink   = pipefs_isymlink,
    .imknod     = pipefs_imknod,
    .ireaddir   = pipefs_ireaddir,
    .ilink      = pipefs_ilink,
    .irename    = pipefs_irename,
};

static int pipefs_fill_sb(filesystem_t *fs __unused, const char *target,
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
    strncpy(sb->sb_magic0, "pipefs", 10);
    sb->sb_size     = -1;
    sb->sb_root     = droot;
    sb->sb_uio      = (cred_t) {
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

static int pipefs_getsb(filesystem_t *fs, const char *src __unused, const char *target,
                       unsigned long flags, void *data, superblock_t **psbp) {
    return getsb_nodev(fs, target, flags, data, psbp, pipefs_fill_sb);
}

int pipefs_init(void) {
    int err = 0;

    if ((err = fs_create("pipefs", &pipefs_iops, &pipefs)))
        return err;

    pipefs->get_sb = pipefs_getsb;
    pipefs->mount = NULL;

    if ((err = vfs_register_fs(pipefs))) {
        fsunlock(pipefs);
        goto error;
    }

    fsunlock(pipefs);
    return 0;
error:
    if (pipefs)
        fs_free(pipefs);
    return err;
}

int pipefs_isync(inode_t *ip __unused) {
    return -ENOSYS;
}

int pipefs_iclose(inode_t *ip __unused) {
    return -ENOSYS;
}

int pipefs_iunlink(inode_t *ip __unused) {
    return -ENOSYS;
}

int pipefs_itruncate(inode_t *ip __unused) {
    return -ENOSYS;
}

int pipefs_igetattr(inode_t *ip __unused, void *attr __unused) {
    return -ENOSYS;
}

int pipefs_isetattr(inode_t *ip __unused, void *attr __unused) {
    return -ENOSYS;
}

int pipefs_ifcntl(inode_t *ip __unused, int cmd __unused, __unused void *argp __unused) {
    return -ENOSYS;
}

int pipefs_iioctl(inode_t *ip __unused, int req __unused, void *argp __unused) {
    return -ENOSYS;
}

ssize_t pipefs_iread_data(inode_t *ip __unused, off_t off __unused, void *buf __unused, size_t nb __unused) {
    return -ENOSYS;
}

ssize_t pipefs_iwrite_data(inode_t *ip __unused, off_t off __unused, void *buf __unused, size_t nb __unused) {
    return -ENOSYS;
}

int pipefs_imkdir(inode_t *dir __unused, const char *fname __unused, mode_t mode __unused) {
    return -ENOSYS;
}

int pipefs_icreate(inode_t *dir __unused, const char *fname __unused, mode_t mode __unused) {
    return -ENOSYS;
}

int pipefs_ibind(inode_t *dir __unused, struct dentry *dentry __unused, inode_t *ip __unused) {
    return -ENOSYS;
}

int pipefs_ilookup(inode_t *dir __unused, const char *fname __unused, inode_t **pipp __unused) {
    return -ENOSYS;
}

int pipefs_isymlink(inode_t *ip __unused, inode_t *atdir __unused, const char *symname __unused) {
    return -ENOSYS;
}

int pipefs_imknod(inode_t *dir __unused, const char *name __unused, mode_t mode __unused, int devid __unused) {
    return -ENOSYS;
}

ssize_t pipefs_ireaddir(inode_t *dir __unused, off_t off __unused, struct dirent *buf __unused, size_t count __unused) {
    return -ENOSYS;
}

int pipefs_ilink(const char *oldname __unused, inode_t *dir __unused, const char *newname __unused) {
    return -ENOSYS;
}

int pipefs_irename(inode_t *dir __unused, const char *old __unused, inode_t *newdir __unused, const char *new __unused) {
    return -ENOSYS;
}