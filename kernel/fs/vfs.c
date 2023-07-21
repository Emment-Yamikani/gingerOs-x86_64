#include <fs/fs.h>
#include <bits/errno.h>
#include <mm/kalloc.h>
#include <lib/printk.h>
#include <lib/string.h>

static LIST_HEAD(fs_list);
static spinlock_t *fs_listlock = &SPINLOCK_INIT();

static volatile unsigned long fs_ID = 0;
static spinlock_t *fs_IDlock = &SPINLOCK_INIT();

static dentry_t *droot = NULL;
static spinlock_t *droot_lock = &SPINLOCK_INIT();

int vfs_init(void)
{
    int err = -EINVAL;

    if (!(droot= dentry_alloc("/")))
        goto error;

    if ((err = devfs_init()))
        goto error;

    if ((err = tmpfs_init()))
        goto error;
    
    dentry_unlock(droot);
    return 0;
error:
    printk("failed to initialize the VFS!\n");
    return err;
}

static dentry_t *vfs_get_droot(void)
{
    dentry_t *dentry = NULL;

    spin_lock(droot_lock);

    if (!droot)
        goto done;

    dentry_lock(droot);
    if (dentry_dup(droot))
    {
        dentry_unlock(droot);
        goto done;
    }

    dentry = droot;

done:
    spin_unlock(droot_lock);
    return dentry;
}

int vfs_set_droot(dentry_t *dentry) {
    spin_lock(droot_lock);
    droot = dentry;
    spin_unlock(droot_lock);
    return 0;
}

int vfs_filesystem_get(const char *type, filesystem_t **pfs) {
    filesystem_t *fs = NULL, *next = NULL;

    spin_lock(fs_listlock);

    list_for_each_entry_safe(fs, next, &fs_list, fs_list) {
        fs_lock(fs);
        if (!compare_strings(type, fs->fs_type)) {
            *pfs = fs;
            fs->fs_count++;
            spin_unlock(fs_listlock);
            return 0;
        }
        fs_unlock(fs);
    }

    spin_unlock(fs_listlock);

    return -ENOENT;
}

void vfs_filesystem_put(filesystem_t *fs) {
    fs_assert_locked(fs);
    fs->fs_count--;
}

int vfs_filesystem_unregister(filesystem_t *fs)
{
    int err = 0;

    (void)fs;

    return 0;
    return err;
}

int vfs_filesystem_alloc(const char *type, filesystem_t **pfs) {
    int err = -ENOMEM;
    char *fstype = NULL;
    filesystem_t *fs = NULL;

    if (!type)
        return -EINVAL;

    if (!(fs = kcalloc(1, sizeof *fs)))
        goto error;

    if (!(fstype = strdup(type)))
        goto error;

    INIT_LIST_HEAD(&fs->fs_list);
    INIT_LIST_HEAD(&fs->fs_sblocks);

    fs->fs_count = 1;
    fs->fs_type = fstype;
    fs->fs_lock = SPINLOCK_INIT();
    fs_lock(fs);

    *pfs = fs;
    return 0;
error:
    if (fs) {
        if (fstype)
            kfree(fstype);
        kfree(fs);
    };
    return err;
}

int vfs_filesystem_register(const char *type, uio_t uio __unused, int flags, iops_t *iops, filesystem_t **pfs) {
    int err = 0;
    filesystem_t *fs = NULL;
    
    if ((err = vfs_filesystem_alloc(type, &fs)))
        goto error;

    fs->fs_iops = iops;
    fs->fs_flags |= flags;
    spin_lock(fs_IDlock);
    fs->fs_id = fs_ID++;
    spin_unlock(fs_IDlock);

    spin_lock(fs_listlock);
    list_add(&fs->fs_list, &fs_list);
    spin_unlock(fs_listlock);

    *pfs = fs;
    
    return 0;
error:
    return err;
}

int vfs_lookup(const char *path, UIO uio, int oflags, mode_t mode, int flags __unused, INODE *pinode, dentry_t **pdentry)
{
    int err = 0;
    size_t tok_i = 0;
    char *cwd = NULL, *abspath = NULL;
    INODE i_dir = NULL, inode = NULL;
    dentry_t *dir = droot, *dentry = NULL;
    char **path_tokens = NULL, *last_tok = NULL;

    if (!uio)
        uio = ROOT_UIO();

    if (!uio->u_cwd)
        cwd = "/";
    else
        cwd = uio->u_cwd;

    if ((err = parse_path(path, cwd, &abspath, &path_tokens, &last_tok)))
        goto error;


    err = -EINVAL;
    if (!(dir = vfs_get_droot()))
        goto error;

    if (!compare_strings("/", abspath))
    {
        dentry = dir;
        inode = dentry->d_inode;
        ilock(inode);
        if ((err = check_iperm(dentry->d_inode, uio, oflags)))
        {
            iunlock(dentry->d_inode);
            dentry_release(dentry);
            dentry_unlock(dentry);
            goto error;
        }
        goto found;
    }

    foreach (filename, path_tokens)
    {
        if ((err = dentry_find(dir, filename, &dentry)) == -ENOENT)
        {
            i_dir = dir->d_inode;
            ilock(i_dir);
            goto delegate_lookup;
        }

        dentry_release(dir);
        dentry_unlock(dir);

        if (err != 0)
            goto error;

        if (!compare_strings(last_tok, filename)) {
            inode = dentry->d_inode;
            goto found;
        }

        tok_i++;
        dir = dentry;
        dentry = NULL;
    }

delegate_lookup:
    foreach (filename, &path_tokens[tok_i])
    {
        if (!(dentry = dentry_alloc(filename)))
        {
            err = -ENOMEM;
            iunlock(i_dir);
            dentry_release(dir);
            dentry_unlock(dir);
            goto error;
        }

        err = ilookup(i_dir, dentry);

        if ((err == -ENOENT) && ((oflags & (O_CREAT | __O_TMPFILE))))
        {
            if ((err = icreate(i_dir, dentry, mode)))
            {
                dentry_close(dentry);

                iunlock(i_dir);
                dentry_release(dir);
                dentry_unlock(dir);
                goto error;
            }

            if ((err = ilookup(i_dir, dentry)))
            {
                if (dentry->d_inode)
                    dentry_close(dentry);

                iunlock(i_dir);
                dentry_release(dir);
                dentry_unlock(dir);
                goto error;
            }
        }
        else if (err != 0)
        {
            dentry_close(dentry);

            iunlock(i_dir);
            dentry_release(dir);
            dentry_unlock(dir);
            goto error;
        }

        if ((err = dentry_bind(dir, dentry)))
        {
            dentry_close(dentry);

            iunlock(i_dir);
            dentry_release(dir);
            dentry_unlock(dir);
            goto error;
        }

        iunlock(i_dir);
        dentry_release(dir);
        dentry_unlock(dir);

        inode = dentry->d_inode;
        ilock(inode);

        if (!compare_strings(last_tok, filename))
            goto found;

        dir = dentry;
        i_dir = inode;
        inode = NULL;
        dentry = NULL;
    }

found:
    if (pinode)
    {
        *pinode = inode;
        idup((inode));
    }
    else
        iunlock(inode);

    if (!pdentry)
    {
        dentry_release(dentry);
        dentry_unlock(dentry);
    }
    else
        *pdentry = dentry;

    tokens_free(path_tokens);
    kfree(abspath);

    return 0;
error:
    if (path_tokens)
        tokens_free(path_tokens);
    if (abspath)
        kfree(abspath);
    return err;
}

/* check for file permission */
int check_iperm(inode_t *ip, uio_t *uio, int oflags)
{
    // printk("%s(\e[0;15mip=%p, uio=%p, oflags=%d)\e[0m\n", __func__, ip, uio, oflags);
    if (!ip || !uio)
        return -EINVAL;

    iassert_locked(ip);

    if (uio->u_uid == 0) /* root */
        return 0;

    if (((oflags & O_ACCMODE) == O_RDONLY) || (oflags & O_ACCMODE) != O_WRONLY)
    {
        if (ip->i_uid == uio->u_uid)
        {
            if (ip->i_mode & S_IRUSR)
                goto write_perms;
        }
        else if (ip->i_gid == uio->u_gid)
        {
            if (ip->i_mode & S_IRGRP)
                goto write_perms;
        }
        else
        {
            if (ip->i_mode & S_IROTH)
                goto write_perms;
        }

        return -EACCES;
    }

write_perms:
    if (((oflags & O_ACCMODE) == O_WRONLY) || (oflags & O_ACCMODE) == O_RDWR)
    {
        if (ip->i_uid == uio->u_uid)
        {
            if (ip->i_mode & S_IWUSR)
                goto exec_perms;
        }
        else if (ip->i_gid == uio->u_gid)
        {
            if (ip->i_mode & S_IWGRP)
                goto exec_perms;
        }
        else
        {
            if (ip->i_mode & S_IWOTH)
                goto exec_perms;
        }

        return -EACCES;
    }

exec_perms:
    if ((oflags & O_EXCL))
    {
        if (ip->i_uid == uio->u_uid)
        {
            if (ip->i_mode & S_IXUSR)
                goto done;
        }
        else if (ip->i_gid == uio->u_gid)
        {
            if (ip->i_mode & S_IXGRP)
                goto done;
        }
        else
        {
            if (ip->i_mode & S_IXOTH)
                goto done;
        }
        return -EACCES;
    }
done:
    // printk("%s(): \e[0;12maccess granted\e[0m\n", __func__);
    return 0;
}