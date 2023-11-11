
#include <fs/fs.h>
#include <fs/tmpfs.h>
#include <mm/kalloc.h>
#include <bits/errno.h>
#include <fs/tmpfs.h>
#include <lib/string.h>
#include <fs/devtmpfs.h>

char *itype_strings[] = {
    [FS_INV] = "INV",
    [FS_RGL] = "REG",
    [FS_DIR] = "DIR",
    [FS_CHR] = "CHR",
    [FS_SYM] = "SYM",
    [FS_BLK] = "BLK",
    [FS_FIFO]= "FIFO",
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
    if ((err = dbind(parent, dnt)))
    {
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
    int err = 0;

    if ((err = dalloc("/", &droot)))
        return err;
    
    dunlock(droot);

    if ((err = ramfs_init()))
        return err;

    if ((err = tmpfs_init()))
        return err;
    
    if ((err = devtmpfs_init()))
        return err;

    if ((err = vfs_mount(NULL, "/", "tmpfs", 0, NULL)))
        return err;

    if ((err = vfs_lookup("/dev/", NULL, O_RDWR | O_CREAT | O_DIRECTORY, 0755, 0, NULL)))
        return err;

    if ((err = vfs_lookup("/mnt/", NULL, O_RDWR | O_CREAT | O_DIRECTORY, 0755, 0, NULL)))
        return err;

    if ((err = vfs_lookup("/tmp/", NULL, O_RDWR | O_CREAT | O_DIRECTORY, 0755, 0, NULL)))
        return err;

    if ((err = vfs_lookup("/ramfs/", NULL, O_RDWR | O_CREAT | O_DIRECTORY, 0755, 0, NULL)))
        return err;

    if ((err = vfs_mount("ramdisk", "/ramfs/", "ramfs", 0, NULL)))
        return err;

    if ((err = vfs_mount(NULL, "/dev/", "devtmpfs", 0, NULL)))
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

    if ((err = ialloc(&ip)))
        return err;

    if ((err = dalloc(name, &dp)))
    {
        irelease(ip);
        return err;
    }

    if ((err = iadd_alias(ip, dp)))
    {
        irelease(ip);
        dclose(dp);
        return err;
    }

    ip->i_type = type;

    *pip = ip;
    *pdp = dp;

    return 0;
}

int vfs_lookup(const char *fn, cred_t *__cred, int oflags, mode_t mode, int flags, dentry_t **pdp) {
    size_t      tok_i = 0;
    char        *cwd = NULL;
    inode_t     *ip = NULL;
    int         err = 0, isdir = 0;
    dentry_t    *dir = NULL, *dp = NULL;
    cred_t      cred = __cred ? *__cred : UIO_DEFAULT();
    char        *path = NULL, *last_tok = NULL, **toks = NULL;

    (void)flags;

    if ((dir = vfs_getdroot()) == NULL)
        return -ENOENT;

    if (cred.c_cwd)
        cwd = "/";
    else
        cwd = "/";

    if ((err = verify_path(fn)))
        return err;
    
    if ((err = parse_path(fn, cwd, &path, &toks, &last_tok, NULL)))
        return err;

    if (!compare_strings(path, "/")) {
        dp = dir;
        goto found;
    }

    foreach(tok, toks) {
        dp = NULL;
        switch ((err = dlookup(dir, tok, &dp))) {
        case 0:
            ilock(dp->d_inode);
            if ((err = check_iperm(dp->d_inode, &cred, oflags))) {
                iunlock(dp->d_inode);
                dclose(dp);
                goto error;
            }
            iunlock(dp->d_inode);
            goto next;
        case -ENOENT:
            goto delegate;
        default:
            dclose(dir);
            goto error;
        }

    next:
        dclose(dir);
        if (!compare_strings(tok, last_tok)) {
            if (isdir && dp->d_inode) {
                ilock(dp->d_inode);
                if (IISDIR(dp->d_inode) == 0) {
                    err = -ENOTDIR;
                    iunlock(dp->d_inode);
                    dclose(dp);
                    goto error;
                }
                iunlock(dp->d_inode);
            }
            goto found;
        }
        dir = dp;
        tok_i++;
    }

delegate:
    dp = NULL;
    foreach(tok, &toks[tok_i]) {
        ilock(dir->d_inode);
    try_lookup:
        // printk("delegate looking up '\e[0;013m%s\e[0m' in '\e[0;013m%s\e[0m'\n", tok, dir->d_name);
        switch ((err = ilookup(dir->d_inode, tok, &ip))) {
        case 0:
            // printk("file(\e[0;013m%s\e[0m) found.\n", tok);
            break;
        case -ENOENT:
            // printk("file(\e[0;013m%s\e[0m) not found.\n", tok);
            // Did user specify O_CREAT flag?
            if ((oflags & O_CREAT)) {
                if (oflags & O_DIRECTORY) {
                    if ((err = imkdir(dir->d_inode, tok, mode))) {
                        iunlock(dir->d_inode);
                        dclose(dir);
                        goto error;
                    }
                } else if ((err = icreate(dir->d_inode, tok, mode))) {
                // create a regular file.
                    iunlock(dir->d_inode);
                    dclose(dir);
                    goto error;
                }

                goto try_lookup;   
            }
            __fallthrough;
        default:
            iunlock(dir->d_inode);
            dclose(dir);
            goto error;
        }
        iunlock(dir->d_inode);

        if ((err = check_iperm(ip, &cred, oflags))) {
            irelease(ip);
            dclose(dir);
            goto error;
        }

        if ((err = dalloc(tok, &dp))) {
            irelease(ip);
            dclose(dir);
            goto delegate_err;
        }

        if ((err = dbind(dir, dp))) {
            dclose(dp);
            irelease(ip);
            dclose(dir);
            goto delegate_err;
        }
        dclose(dir);

        if ((err = iadd_alias(ip, dp))) {
            dclose(dp);
            irelease(ip);
            goto delegate_err;
        }
        irelease(ip);

        dir = dp;
    }

found:
    if (pdp)
        *pdp = dp;
    else
        dclose(dp);
    return 0;
error:
    return err;
delegate_err:
    return err;
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

    if ((err = vfs_lookup(path, NULL, O_RDONLY, 0, 0, &dfile)))
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