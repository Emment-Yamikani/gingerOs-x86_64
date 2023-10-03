
#include <fs/fs.h>
#include <fs/tmpfs.h>
#include <mm/kalloc.h>
#include <bits/errno.h>
#include <fs/tmpfs.h>

static dentry_t *droot = NULL;
static queue_t *fs_queue = &QUEUE_INIT();

dentry_t *vfs_getdroot(void) {
    if (droot) {
        dlock(droot);
        ddup(droot);
    }
    return droot;
}

int vfs_mount_droot(dentry_t *dentry) {
    if (dentry == NULL)
        return -EINVAL;
    droot = dentry;
    return 0;
}

int vfs_init(void) {
    int err = 0;
    inode_t *ip = NULL;
    dentry_t *dnt = NULL;

    if ((err = dalloc("/", &droot)))
        return err;
    
    dunlock(droot);

    if ((err = ramfs_init()))
        return err;

    if ((err = tmpfs_init()))
        return err;

    if ((err = vfs_mount("ramdisk", "/", "ramfs", 0, NULL)))
        return err;

    if ((err = vfs_alloc_vnode("tmp", FS_DIR, &ip, &dnt)))
        return err;
    
    dlock(droot);
    if ((err = dbind(droot, dnt))){
        dclose(dnt);
        iclose(ip);
        dunlock(droot);
        return err;
    }

    dunlock(droot);

    dunlock(dnt);
    iunlock(ip);

    if ((err = vfs_mount(NULL, "/tmp", "tmpfs", 0, NULL)))
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

int vfs_lookup(const char *fn, uio_t *__uio,
               int oflags, mode_t mode __unused,
               int flags __unused, inode_t **pip, dentry_t **pdp) {
    size_t tok_i = 0;
    char *cwd = NULL;
    inode_t *ip = NULL;
    int err = 0, isdir = 0;
    dentry_t *d_dir = NULL, *dp = NULL;
    uio_t uio = __uio ? *__uio : UIO_DEFAULT();
    char *path = NULL, *last_tok = NULL, **toks = NULL;

    if ((d_dir = vfs_getdroot()) == NULL)
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
        dp = d_dir;
        goto found;
    }

    foreach(tok, toks) {
        dp = NULL;
        if ((err = dlookup(d_dir, tok, &dp)) == 0)
            goto next;
        else if (err == -ENOENT)
            goto delegate;
        else {
            dunlock(d_dir);
            goto error;
        }
    next:
        dunlock(d_dir);
        if (!compare_strings(tok, last_tok)) {
            if (isdir) {
                if (dp->d_inode) {
                    ilock(dp->d_inode);
                    if (IISDIR(dp->d_inode) == 0) {
                        err = -ENOTDIR;
                        iunlock(dp->d_inode);
                        dunlock(dp);
                        goto error;
                    }
                    iunlock(dp->d_inode);
                }
            }
            goto found;
        }
        d_dir = dp;
        tok_i++;
    }

delegate:
    dp = NULL;
    foreach(tok, &toks[tok_i]) {
        printk("looking up '%s' in '%s'\n", tok, d_dir->d_name);
        if ((err = dalloc(tok, &dp))) {
            dunlock(d_dir);
            goto error;
        }

        ilock(d_dir->d_inode);
        if ((err = ilookup(d_dir->d_inode, dp))) {
            dunlock(d_dir);
            iunlock(d_dir->d_inode);
            goto error;
        }
        iunlock(d_dir->d_inode);

        if ((err = dbind(d_dir, dp))) {
            iunlock(dp->d_inode);
            dclose(dp);
            dunlock(d_dir);
            goto error;
        }

        dunlock(d_dir);
        d_dir = dp;
        ip = dp->d_inode;

        if ((err = check_iperm(ip, &uio, oflags))) {
            iunlock(ip);
            dunlock(dp);
            goto error;
        }

        printk("comparing...\n");
        if (compare_strings(tok, last_tok))
            iunlock(ip);
    }

found:
    if (pdp) {
        ddup(dp);
        *pdp = dp;
    }
    else
        dclose(dp);
    
    if (pip) {
        idupcnt(ip);
        *pip = ip;
    }
    else if (ip)
        iputcnt(ip);
    
    printk("%s() done...\n", __func__);
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
