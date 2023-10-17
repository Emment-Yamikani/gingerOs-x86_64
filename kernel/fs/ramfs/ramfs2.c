#include <fs/fs.h>
#include <lib/printk.h>
#include <dev/dev.h>
// #include <fs/posix.h>
#include <sys/_fcntl.h>
#include <fs/ramfs2.h>
#include <mm/kalloc.h>
#include <bits/errno.h>
#include <lib/string.h>

static iops_t ramfs2_iops;
static inode_t *iroot = NULL;
static superblock_t *ramfs2_sb = NULL;
static ramfs2_super_t *ramfs2_super = NULL;
// static vmr_ops_t ramfs2_vmr_ops __unused;

static int ramfs_getsb(filesystem_t *fs, const char *src, const char *target, unsigned long flags, void *data, superblock_t **);

static int ramfs_fill_sb(filesystem_t *fs, const char *target, struct devid *devid, superblock_t *sb);

filesystem_t ramfs2 = {
    .fs_iops = &ramfs2_iops,
    .fs_id = 0,
    .fs_count = 1,
    .fs_flags = 0,
    .fs_priv = NULL,
    .fs_name = "ramfs",
    .fs_lock = SPINLOCK_INIT(),
    .get_sb = ramfs_getsb,
    .fs_superblocks = &QUEUE_INIT(),
};

int ramfs_init(void)
{
    int err = 0;
    fslock(&ramfs2);
    if ((err = vfs_register_fs(&ramfs2)))
    {
        fsunlock(&ramfs2);
        return err;
    }
    fsunlock(&ramfs2);
    return err;
}

static int ramfs_getsb(filesystem_t *fs, const char *src,
            const char *target, unsigned long flags,
            void *data, superblock_t **psb)
{
    return getsb_bdev(fs, src, target, flags, data, psb, ramfs_fill_sb);
}

static int ramfs_fill_sb(filesystem_t *fs, const char *target, 
                    struct devid *devid, superblock_t *sb)
{
    ssize_t err = 0;
    size_t sbsz = 0;
    dentry_t *droot = NULL;
    ramfs2_super_header_t hdr = {0};

    sbassert_locked(sb);

    if (fs == NULL || devid == NULL || sb == NULL)
        return -EINVAL;

    if ((kdev_read(devid, 0, &hdr, sizeof hdr)) != sizeof hdr)
        return -EFAULT;

    sbsz = sizeof hdr + (hdr.nfile * sizeof(ramfs2_node_t));

    if ((ramfs2_super = kcalloc(1, sbsz)) == NULL)
        return -ENOMEM;


    if ((err = kdev_read(devid, 0, ramfs2_super, sbsz)) < 0)
        return err;

    if ((err = ramfs2_validate(ramfs2_super)))
        return err;

    if ((err = ialloc(&iroot)))
        return err;

    if ((err = dalloc(target, &droot))) {
        iclose(iroot);
        return err;
    }

    if ((err = iadd_alias(iroot, droot))) {
        dclose(droot);
        iclose(iroot);
        return err;
    }

    sb->sb_blocksize = 512;
    strncpy(sb->sb_magic0, ramfs2_super->header.magic,
            strlen(ramfs2_super->header.magic));
    sb->sb_size = ramfs2_super->header.file_size;
    sb->sb_uio  = (uio_t) {
        .u_cwd = "/",
        .u_root = "/",
        .u_gid = 0,
        .u_uid = 0,
        .u_umask = 0555,
    };
    sb->sb_root = droot;
    ramfs2_sb = sb;

    iroot->i_type = FS_DIR;
    iroot->i_sb = ramfs2_sb;
    iroot->i_ops = ramfs2_sb->sb_iops;
    dunlock(droot);
    iunlock(iroot);

    return 0;
}

int ramfs2_validate(ramfs2_super_t *super)
{
    uint32_t chksum = 0;
    const char *magic = "ginger_rd2";
    if (!super)
        return -EINVAL;
    if (compare_strings(super->header.magic, (char *)magic))
        return -EINVAL;
    for (int i = 0; i < __magic_len; ++i)
        chksum += (uint32_t)super->header.magic[i];
    chksum += super->header.ramfs2_size + super->header.super_size + super->header.checksum;
    for (uint32_t i = 0; i < super->header.nfile; ++i)
        chksum += super->nodes[i].size;
    return chksum;
}

static ramfs2_node_t *ramfs2_convert_inode(inode_t *ip)
{
    if (ip == NULL) return NULL;
    return ip->i_priv;
}

int ramfs2_find(ramfs2_super_t *super, const char *fn, ramfs2_node_t **pnode)
{
    if (!super || !pnode)
        return -EINVAL;

    if (!fn || !*fn)
        return -ENOTNAM;

    if ((strlen(fn) >= __max_fname))
        return -ENAMETOOLONG;

    for (uint32_t indx = 0; indx < super->header.nfile; ++indx)
    {
        if (!compare_strings(super->nodes[indx].name, (char *)fn))
        {
            *pnode = &super->nodes[indx];
            return 0;
        }
    }

    return -ENOENT;
}

static int ramfs_ilookup(inode_t *dir, dentry_t *dentry) {
    int err = 0;
    inode_t *ip = NULL;
    ramfs2_node_t *node = NULL;

    if (!dir || dentry == NULL)
        return -EINVAL;

    if (IISDIR(dir) == 0)
        return -ENOTDIR;

    if ((err = ramfs2_find(ramfs2_super, dentry->d_name, &node)))
        return err;
    if ((err = ialloc(&ip)))
        return err;

    ip->i_sb = ramfs2_sb;
    ip->i_ops = ramfs2_sb->sb_iops;
    ip->i_priv = node;
    ip->i_gid = node->gid;
    ip->i_uid = node->uid;
    ip->i_mode = node->mode;
    ip->i_size = node->size;
    ip->i_type = (int[]) {
        [RAMFS2_INV] = FS_INV,
        [RAMFS2_REG] = FS_RGL,
        [RAMFS2_DIR] = FS_DIR,
    }[node->type];
    ip->i_ino = node - ramfs2_super->nodes;

    if ((err = iadd_alias(ip, dentry))) {
        iclose(ip);
        return err;
    }

    return 0;
};

__unused static int ramfs2_open(inode_t *ip __unused, int mode __unused, ...)
{
    return 0;
}

static ssize_t ramfs2_read(inode_t *ip, off_t off, void *buf, size_t sz)
{
    ramfs2_node_t *node = NULL;
    ssize_t retval = 0;

    if (!ip || !buf)
        return -EINVAL;
    if ((node = ramfs2_convert_inode(ip)) == NULL)
        return -EINVAL;
    if (off >= node->size)
        return -1;
    sz = MIN((node->size - off), sz);
    off += node->offset + ramfs2_super->header.data_offset;
    
    sblock(ip->i_sb);
    retval = kdev_read(&ip->i_sb->sb_devid, off, buf, sz);
    sbunlock(ip->i_sb);
    return retval;
}

static ssize_t ramfs2_write(inode_t *ip __unused, off_t off __unused, void *buf __unused, size_t sz __unused)
{
    return -EROFS;
}

static int ramfs2_close(inode_t *ip __unused)
{
    return 0;
}

static int ramfs2_creat(inode_t *ip __unused, dentry_t *dentry __unused, int mode __unused)
{
    return -EROFS;
}

static int ramfs2_sync(inode_t *ip __unused)
{
    return -EROFS;
}

static int ramfs2_ioctl(inode_t *ip __unused, int req __unused, void *argp __unused)
{
    return -ENOTTY;
}

__unused static int ramfs2_lseek(inode_t *ip __unused, off_t off __unused, int whence __unused)
{
    return -EINVAL;
}

static ssize_t ramfs2_readdir(inode_t *dir __unused, off_t offset __unused, struct dirent *dirent __unused, size_t count __unused)
{
    if (iroot != dir)
        return -EINVAL;
    return -ENOSYS;
}

__unused static int ramfs2_chown(inode_t *ip __unused, uid_t uid __unused, gid_t gid __unused)
{
    return -EROFS;
}

/*
int ramfs2_fault(vmr_t *vmr __unused, struct vm_fault *fault __unused)
{
    return -ENOSYS;
}

int ramfs2_mmap(file_t *file __unused, vmr_t *vmr __unused)
{
    return -ENOSYS;
}
*/

static iops_t ramfs2_iops = {
    .iclose = ramfs2_close,
    .icreate = ramfs2_creat,
    .ilookup = ramfs_ilookup,
    .ibind = NULL,
    // .ichown = ramfs2_chown,
    .iioctl = ramfs2_ioctl,
    // .ilseek = ramfs2_lseek,
    // .iopen = ramfs2_open,
    .iread = ramfs2_read,
    .isync = ramfs2_sync,
    .iwrite = ramfs2_write,
    .ireaddir = ramfs2_readdir,
};

/*
static struct fops ramfs2_fops = (struct fops){
    .can_read = posix_file_can_read,
    .can_write = posix_file_can_write,
    .close = posix_file_close,
    .open = posix_file_open,
    .eof = posix_file_eof,
    .ioctl = posix_file_ioctl,
    .lseek = posix_file_lseek,
    .read = posix_file_read,
    .write = posix_file_write,
    .readdir = posix_file_readdir,
    .stat = posix_file_ffstat,
    .mmap = ramfs2_mmap,
};
*/