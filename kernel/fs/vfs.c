#include <bits/errno.h>
#include <fs/fs.h>
#include <fs/devtmpfs.h>
#include <fs/tmpfs.h>
#include <fs/pipefs.h>
#include <fs/procfs.h>
#include <fs/sysfs.h>
#include <fs/tmpfs.h>
#include <lib/string.h>
#include <mm/kalloc.h>

char *itype_strings[] = {
    [FS_INV] = "INV",
    [FS_RGL] = "REG",
    [FS_DIR] = "DIR",
    [FS_CHR] = "CHR",
    [FS_LNK] = "LNK",
    [FS_BLK] = "BLK",
    [FS_FIFO]= "FIFO",
    [FS_PIPE]= "PIPE"
};

static dentry_t *droot = NULL;
static queue_t *fs_queue = &QUEUE_INIT();

dentry_t *vfs_getdroot(void) {
    if (droot) {
        dlock(droot);
        dopen(droot);
    }
    return droot;
}

/// returns 1 if dentry is the root of vfs and zero otherwise.
int vfs_isroot(dentry_t *dentry) {
    return dentry ? droot ? dentry == droot : 0 : 0; 
}

int vfs_mount_droot(dentry_t *dentry) {
    if (dentry == NULL)
        return -EINVAL;
    droot = dentry;
    return 0;
}

int vfs_mkpauedo_dir(const char *name, dentry_t *parent) {
    int err = 0;
    inode_t *ip = NULL;
    dentry_t *dnt = NULL;
    if ((err = vfs_alloc_vnode(name, FS_DIR, &ip, &dnt)))
        return err;

    dlock(parent);
    if ((err = dbind(parent, dnt))) {
        dunlock(parent);
        dclose(dnt);
        irelease(ip);
        return err;
    }

    dunlock(parent);

    dunlock(dnt);
    iunlock(ip);

    return 0;
}

int vfs_init(void) {
    int         err    = 0;
    mode_t      mode   = 0;
    const char  *dir[] = {
        "/dev/", "/mnt/", "/tmp/",
        "/ramfs/", "/proc/", "/sys/", NULL,
    };

    const char *dev_dirs[] = {
        "/dev/block", "/dev/bus", "/dev/char", "/dev/cpu", "/dev/disk",
        "/dev/input", "/dev/net", "/dev/pts", "/dev/shm", NULL
    };

    if ((err = dalloc("/", &droot)))
        return err;

    dunlock(droot);

    if ((err = ramfs_init()))
        return err;

    if ((err = tmpfs_init()))
        return err;

    if ((err = devtmpfs_init()))
        return err;

    if ((err = pipefs_init()))
        return err;

    if ((err = procfs_init()))
        return err;

    if ((err = sysfs_init()))
        return err;

    if ((err = vfs_mount(NULL, "/", "tmpfs", 0, NULL)))
        return err;

    mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

    for (int i = 0; dir[i] ; ++i) {
        if ((err = vfs_mkdir(dir[i], NULL, mode | S_IFDIR)))
            return err;
    }

    if ((err = vfs_mount("ramdisk0", "/ramfs/", "ramfs", 0, NULL)))
        return err;

    if ((err = vfs_mount(NULL, "/dev/", "devtmpfs", 0, NULL)))
        return err;
    
    for (int i = 0; dev_dirs[i] ; ++i) {
        if ((err = vfs_mkdir(dev_dirs[i], NULL, mode | S_IFDIR)))
            return err;
    }

    if ((err = vfs_mount(NULL, "/sys/", "sysfs", 0, NULL)))
        return err;

    if ((err = vfs_mount(NULL, "/proc/", "procfs", 0, NULL)))
        return err;

    return 0;
}

int vfs_alloc_vnode(const char *name, itype_t type, inode_t **pip, dentry_t **pdp) {
    int err = 0;
    inode_t *ip = NULL;
    dentry_t *dp = NULL;

    if ((err = verify_path(name)))
        return err;
    
    if (pip == NULL || pdp == NULL)
        return -EINVAL;

    if ((err = ialloc(type, 0, &ip)))
        return err;

    if ((err = dalloc(name, &dp))) {
        irelease(ip);
        return err;
    }

    if ((err = iadd_alias(ip, dp))) {
        irelease(ip);
        dclose(dp);
        return err;
    }

    ip->i_type = type;

    *pip = ip;
    *pdp = dp;

    return 0;
}

int vfs_register_fs(filesystem_t *fs) {
    int err = 0;

    fsassert_locked(fs);
    if (fs == NULL)
        return -EINVAL;

    queue_lock(fs_queue);

    err = enqueue(fs_queue, fs, 1, NULL);

    queue_unlock(fs_queue);
    
    
    return err;
}

int vfs_unregister_fs(filesystem_t *fs) {
    fsassert_locked(fs);
    if (fs == NULL)
        return -EINVAL;

    if (fs_count(fs) > 0)
        return -EBUSY;
    
    return -EBUSY;
}

int vfs_getfs(const char *type, filesystem_t **pfs) {
    filesystem_t *fs = NULL;
    queue_node_t *next = NULL;

    if (type == NULL || pfs == NULL)
        return -EINVAL;

    queue_lock(fs_queue);

    forlinked(node, fs_queue->head, next) {
        fs = node->data;
        next = node->next;

        fslock(fs);
        if (!compare_strings(type, fs->fs_name)) {
            *pfs = fs;
            queue_unlock(fs_queue);
            return 0;
        }
        fsunlock(fs);
    }

    queue_unlock(fs_queue);
    return -ENOENT;
}

int vfs_dirlist(const char *path) {
    int err = 0;
    off_t off = 0;
    dentry_t *dfile = NULL;
    struct dirent dp = {0};

    if ((err = vfs_lookup(path, NULL, O_RDONLY, &dfile)))
        return err;

    printk("%-16s %9s %9s %7s\n", "Name", "I-num", "Size", "Type");

    ilock(dfile->d_inode);
    while ((0 == ireaddir(dfile->d_inode, off++, &dp, 1))) {
        if (dp.d_type == FS_RGL)
            printk("\e[0;011m%-16s\e[0m \e[0;03m%9ld\e[0m "
                    "\e[0;04m%9ld\e[0m \e[0;08m%7s\e[0m\n",
                   dp.d_name, dp.d_ino, dp.d_size, itype_strings[dp.d_type]);
        if (dp.d_type == FS_DIR)
            printk("\e[0;03m%-16s\e[0m \e[0;03m%9ld\e[0m "
                    "\e[0;04m%9ld\e[0m \e[0;06m%7s\e[0m\n",
                   dp.d_name, dp.d_ino, dp.d_size, itype_strings[dp.d_type]);
        if (dp.d_type == FS_CHR || dp.d_type == FS_BLK)
            printk("\e[0;02m%-16s\e[0m \e[0;03m%9ld\e[0m "
                    "\e[0;04m%9ld\e[0m \e[0;010m%7s\e[0m\n",
                   dp.d_name, dp.d_ino, dp.d_size, itype_strings[dp.d_type]);
    }
    iunlock(dfile->d_inode);
    dclose(dfile);

    return 0;
}