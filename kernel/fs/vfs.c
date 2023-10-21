
#include <fs/fs.h>
#include <fs/tmpfs.h>
#include <mm/kalloc.h>
#include <bits/errno.h>
#include <fs/tmpfs.h>
#include <sys/_fcntl.h>

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

static int vfs_mkpauedo_dir(const char *name, dentry_t *parent) {
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
        iclose(ip);
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

    if ((err = vfs_mount("ramdisk", "/", "ramfs", 0, NULL)))
        return err;

    if ((err = vfs_mkpauedo_dir("dev", droot)))
        return err;

    if ((err = vfs_mkpauedo_dir("tmp", droot)))
        return err;
    
    if ((err = vfs_mkpauedo_dir("mnt", droot)))
        return err;

    if ((err = vfs_mount(NULL, "/tmp/", "tmpfs", 0, NULL)))
        return err;

    if ((err = vfs_mount(NULL, "/dev/", "tmpfs", 0, NULL)))
        return err;

    if ((err = vfs_mount(NULL, "/mnt/", "tmpfs", 0, NULL)))
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
        iclose(ip);
        return err;
    }

    if ((err = iadd_alias(ip, dp)))
    {
        iclose(ip);
        dclose(dp);
        return err;
    }

    ip->i_type = type;

    *pip = ip;
    *pdp = dp;

    return 0;
}

int vfs_lookup(const char *fn, uio_t *__uio, int oflags, mode_t mode, int flags, dentry_t **pdp) {
    size_t tok_i = 0;
    char *cwd = NULL;
    inode_t *ip = NULL;
    int err = 0, isdir = 0;
    dentry_t *dir = NULL, *dp = NULL;
    uio_t uio = __uio ? *__uio : UIO_DEFAULT();
    char *path = NULL, *last_tok = NULL, **toks = NULL;

    (void)flags;

    if ((dir = vfs_getdroot()) == NULL)
        return -ENOENT;

    if (uio.u_cwd)
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
        printk("delegate looking up '%s' in '%s'\n", tok, dir->d_name);
        switch ((err = ilookup(dir->d_inode, tok, &ip))) {
        case 0:
            break;
        case -ENOENT:
            printk("file not found by delegate\n");
            // Did user specify O_CREAT flag?
            if ((oflags & O_CREAT))
                goto creat;
            __fallthrough;
        default:
            iunlock(dir->d_inode);
            dunlock(dir);
            goto error;
        }
        iunlock(dir->d_inode);

        if ((err = dalloc(tok, &dp))) {
            iclose(ip);
            dunlock(dir);
            goto error;
        }

        if ((err = iadd_alias(ip, dp))) {
            iclose(ip);
            dunlock(dir);
            goto error;
        }

        if ((err = dbind(dir, dp))) {
            iunlock(dp->d_inode);
            dclose(dp);
            dunlock(dir);
            goto error;
        }

        dunlock(dir);
        dir = dp;

        if ((err = check_iperm(dp->d_inode, &uio, oflags))) {
            iunlock(dp->d_inode);
            dunlock(dp);
            goto error;
        }

        printk("comparing...\n");
        if (compare_strings(tok, last_tok))
            iunlock(dp->d_inode);
    }

found:
    if (pdp) {
        ddup(dp);
        *pdp = dp;
    }
    else {
        dclose(dp);
    }
    return 0;

creat:
    // create a directory
    if (oflags & O_DIRECTORY) {
        printk("cpu:%d: creating a directory file\n", cpu_id);
        if ((err = imkdir(dir->d_inode, dp, mode))) {
            iunlock(dp->d_inode);
            dclose(dp);
            dunlock(dir);
            goto error;
        }

        ilock(dp->d_inode);
    } else { // create a regular file.
        printk("create a regular file\n");
    }


    goto found;
    return 0;
error:
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
