#include <fs/fs.h>
#include <fs/tmpfs.h>
#include <lib/string.h>
#include <lib/printk.h>

static filesystem_t *devtmpfs = NULL;

static iops_t devtmpfs_iops = {
    .iopen      = tmpfs_iopen,
    .ibind      = tmpfs_ibind,
    .isync      = tmpfs_isync,
    .ilink      = tmpfs_ilink,
    .iread      = tmpfs_iread,
    .iwrite     = tmpfs_iwrite,
    .iclose     = tmpfs_iclose,
    .ifcntl     = tmpfs_ifcntl,
    .iioctl     = tmpfs_iioctl,
    .imkdir     = tmpfs_imkdir,
    .imknod     = tmpfs_imknod,
    .icreate    = tmpfs_icreate,
    .ilookup    = tmpfs_ilookup,
    .iunlink    = tmpfs_iunlink,
    .irename    = tmpfs_irename,
    .igetattr   = tmpfs_igetattr,
    .isetattr   = tmpfs_isetattr,
    .isymlink   = tmpfs_isymlink,
    .ireaddir   = tmpfs_ireaddir,
    .itruncate  = tmpfs_itruncate,
};

static int devtmpfs_fill_sb(filesystem_t *fs __unused, const char *target,
                         struct devid *devid __unused, superblock_t *sb) {
    int         err     = 0;
    inode_t     *iroot  = NULL;
    dentry_t    *droot  = NULL;

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

    strncpy(sb->sb_magic0, "devtmpfs", 10);
    sb->sb_blocksize = -1;
    sb->sb_size      = -1;
    sb->sb_root      = droot;
    sb->sb_uio = (cred_t){
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

static int devtmpfs_getsb(filesystem_t *fs, const char *src __unused, const char *target,
                       unsigned long flags, void *data, superblock_t **psbp) {
    return getsb_nodev(fs, target, flags, data, psbp, devtmpfs_fill_sb);
}

int devtmpfs_init(void) {
    int err = 0;

    if ((err = fs_create("devtmpfs", &devtmpfs_iops, &devtmpfs)))
        return err;

    devtmpfs->get_sb = devtmpfs_getsb;
    devtmpfs->mount = NULL;

    if ((err = vfs_register_fs(devtmpfs))) {
        fsunlock(devtmpfs);
        goto error;
    }

    fsunlock(devtmpfs);
    return 0;
error:
    if (devtmpfs)
        fs_free(devtmpfs);
    return err;
}