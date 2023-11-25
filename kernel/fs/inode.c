#include <fs/fs.h>
#include <fs/inode.h>
#include <fs/dentry.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <lib/types.h>
#include <mm/kalloc.h>
#include <fs/fcntl.h>
#include <fs/stat.h>

void ifree(inode_t *ip) {
    iassert_locked(ip);

    if (ip->i_refcnt <= 0) {
        iunlink(ip);
        icache_free(ip->i_cache);
        iunlock(ip);
        kfree(ip);
        return;
    }

    iunlock(ip);
}

static int inew(inode_t **pip) {
    int err = -ENOMEM;
    inode_t *ip = NULL;

    if (pip == NULL)
        return -EINVAL;

    if ((ip = kmalloc(sizeof *ip)) == NULL)
        return err;

    memset(ip, 0, sizeof *ip);

    ip->i_refcnt = 1;
    ip->i_lock = SPINLOCK_INIT();
    ip->i_datalock = SPINLOCK_INIT();
    ilock(ip);

    *pip = ip;
    return 0;
}

int     ialloc(itype_t type,  inode_t **pip) {
    int         err = 0;
    inode_t     *ip = NULL;
    icache_t    *icache = NULL;
    cond_t      *reader = NULL;
    cond_t      *writer = NULL;

    if (pip == NULL)
        return -EINVAL;

    if ((err = inew(&ip)))
        return err;

    ip->i_type = type;

    if (IISDEV(ip) == 0) {
        if ((err = icache_alloc(&icache)))
            goto error;
    }

    if ((err - cond_new(&reader)))
        goto error;
    
    if ((err = cond_new(&writer)))
        goto error;

    ip->i_cache = icache;
    icache->pc_inode = ip;
    ip->i_writers = writer;
    ip->i_readers = reader;

    *pip = ip;
    return 0;
error:
    if (ip)
        ifree(ip);
    if (icache)
        icache_free(icache);
    if (reader)
        cond_free(reader);
    if (writer)
        cond_free(writer);
    return err;
}

void idupcnt(inode_t *ip) {
    iassert_locked(ip);
    ip->i_refcnt++;
}

void iputcnt(inode_t *ip) {
    iassert_locked(ip);
    ip->i_refcnt--;
}

void iputlink(inode_t *ip) {
    iassert_locked(ip);
    ip->i_hlinks--;
}

void iduplink(inode_t *ip) {
    iassert_locked(ip);
    ip->i_hlinks++;
}

void irelease(inode_t *ip) {
    iassert_locked(ip);
    idupcnt(ip);
    if (ip->i_refcnt <= 0) {
        iclose(ip);
        printk("ip closed\n");
    } else
        iunlock(ip);
}

int idel_alias(inode_t *inode, dentry_t *dentry) {
    dentry_t *prev = NULL, *next = NULL;

    iassert_locked(inode);
    dassert_locked(dentry);

    if (dentry->d_inode != inode)
        return -EINVAL;
    
    if (inode->i_alias == NULL)
        return -ENOENT;

    prev = dentry->d_alias_prev;
    next = dentry->d_alias_next;

    if (prev)
        dlock(prev);
    if (next)
        dlock(next);

    if (next)
        next->d_alias_prev = prev;

    if (prev)
        prev->d_alias_next = next;
    else
        inode->i_alias = next;

    if (next)
        dunlock(next);
    if (prev)
        dunlock(prev);

    dentry->d_alias_next = NULL;
    dentry->d_alias_prev = NULL;

    irelease(inode);
    return 0;
}

int iadd_alias(inode_t *inode, dentry_t *dentry) {
    dentry_t *last = NULL;
    dentry_t *next = NULL;

    if (inode == NULL || dentry == NULL)
        return -EINVAL;
    
    iassert_locked(inode);
    dassert_locked(dentry);

    forlinked(node, inode->i_alias, next) {
        dlock(node);
        next = node->d_alias_next;
        last = node;
        
        /// unlock node in case we still have a next alias.
        /// this is done so that when the loop ends
        /// the last node in alias list is passed locked.
        if (next)
            dunlock(node);
    }

    dentry->d_alias_next = NULL;
    dentry->d_alias_prev = NULL;

    if (last) {
        last->d_alias_next = dentry;
        dentry->d_alias_prev = last;

        /// Unlock the last alias node.
        /// Remember we locked this node in to forlinked() loop above?,
        /// Yeah, so do this to prevent deadlock.
        dunlock(last);
    } else inode->i_alias = dentry;
    

    /// Increase the reference count to this inode
    /// because we have added an inode alias.
    idupcnt(inode);
    dentry->d_inode = inode;

    return 0;
}

int iopen(inode_t *ip) {
    if (ip == NULL)
        return -EINVAL;
    iassert_locked(ip);

    idupcnt(ip);

    // TODO: add filesystem specific open here if need be.
    return 0;
}

int     ibind(inode_t *dir, struct dentry *dentry, inode_t *ip) {
    int err = 0;
    iassert_locked(dir);
    iassert_locked(ip);
    dassert_locked(dentry);

    if (IISDIR(dir) == 0)
        return -ENOTDIR;
    
    if ((err = icheck_op(dir, ibind)))
        return err;
    
    return dir->i_ops->ibind(dir, dentry, ip);
}

int     isync(inode_t *ip) {
    int err = 0;
    iassert_locked(ip);

    if ((err = icheck_op(ip, isync)))
        return err;
    
    return ip->i_ops->isync(ip);
}

int     ilink(const char *oldname, inode_t *dir, const char *newname) {
    int err = 0;
    iassert_locked(dir);

    if (IISDIR(dir) == 0)
        return -ENOTDIR;

    if ((err = icheck_op(dir, ilink)))
        return err;
    
    return dir->i_ops->ilink(oldname, dir, newname);
}

int     iclose(inode_t *ip) {
    int err = 0;
    iassert_locked(ip);

    if ((err = icheck_op(ip, iclose)))
        return err;
    
    return ip->i_ops->iclose(ip);
}

ssize_t iread_data(inode_t *ip, off_t off, void *buf, size_t nb) {
    ssize_t err = 0;

    iassert_locked(ip);

    if (IISDIR(ip))
        return -EISDIR;

    if ((err = icheck_op(ip, iread)))
        return err;
    
    return ip->i_ops->iread(ip, off, buf, nb);
}

ssize_t iwrite_data(inode_t *ip, off_t off, void *buf, size_t nb) {
    ssize_t err = 0;
    iassert_locked(ip);

    if (IISDIR(ip))
        return -EISDIR;

    if ((err = icheck_op(ip, iwrite)))
        return err;
    
    return ip->i_ops->iwrite(ip, off, buf, nb);
}

int     imknod(inode_t *dir, const char *name, mode_t mode, int devid) {
    int err = 0;
    iassert_locked(dir);

    if (IISDIR(dir) == 0)
        return -ENOTDIR;

    if ((err = icheck_op(dir, imknod)))
        return err;
    
    return dir->i_ops->imknod(dir, name, mode, devid);
}

int     ifcntl(inode_t *ip, int cmd, void *argp) {
    int err = 0;
    iassert_locked(ip);

    if (argp == NULL)
        return -EINVAL;

    if ((err = icheck_op(ip, ifcntl)))
        return err;
    
    return ip->i_ops->ifcntl(ip, cmd, argp);
}

int     iioctl(inode_t *ip, int req, void *argp) {
    int err = 0;
    iassert_locked(ip);

    if (argp == NULL)
        return -EINVAL;

    if (IISDIR(ip))
        return -EISDIR;

    if ((err = icheck_op(ip, iioctl)))
        return err;
    
    return ip->i_ops->iioctl(ip, req, argp);
}

int     imkdir(inode_t *dir, const char *fname, mode_t mode) {
    int err = 0;
    iassert_locked(dir);

    if (IISDIR(dir) == 0)
        return -ENOTDIR;

    if ((err = icheck_op(dir, imkdir)))
        return err;

    return dir->i_ops->imkdir(dir, fname, mode);
}

int     iunlink(inode_t *ip) {
    int err = 0;
    iassert_locked(ip);

    if ((err = icheck_op(ip, iunlink)))
        return err;
    
    return ip->i_ops->iunlink(ip);
}

int     ilookup(inode_t *dir, const char *fname, inode_t **pipp) {
    int err = 0;
    
    iassert_locked(dir);

    if (IISDIR(dir) == 0)
        return -ENOTDIR;

    if ((err = icheck_op(dir, ilookup)))
        return err;
    
    return dir->i_ops->ilookup(dir, fname, pipp);
}

int     icreate(inode_t *dir, const char *fname, mode_t mode) {
    int err = 0;
    iassert_locked(dir);

    if (IISDIR(dir) == 0)
        return -ENOTDIR;

    if ((err = icheck_op(dir, icreate)))
        return err;
    
    return dir->i_ops->icreate(dir, fname, mode);
}

int     irename(inode_t *dir, const char *old, inode_t *newdir, const char *new) {
    int err = 0;
    iassert_locked(dir);
    iassert_locked(newdir);

    if ((err = icheck_op(dir, irename)))
        return err;
    
    return dir->i_ops->irename(dir, old, newdir, new);
}

ssize_t ireaddir(inode_t *dir, off_t off, void *buf, size_t count) {
    ssize_t err = 0;
    iassert_locked(dir);

    if (IISDIR(dir) == 0)
        return -ENOTDIR;

    if (buf == NULL)
        return -EINVAL;

    if ((err = icheck_op(dir, ireaddir)))
        return err;
    
    return dir->i_ops->ireaddir(dir, off, buf, count);
}

int     isymlink(inode_t *ip, inode_t *atdir, const char *symname) {
    int err = 0;
    iassert_locked(ip);
    if ((err = icheck_op(ip, isymlink)))
        return err;
    
    return ip->i_ops->isymlink(ip, atdir, symname);
}

int     igetattr(inode_t *ip, void *attr) {
    int err = 0;
    iassert_locked(ip);
    if ((err = icheck_op(ip, igetattr)))
        return err;
    
    return ip->i_ops->igetattr(ip, attr);
}

int     isetattr(inode_t *ip, void *attr) {
    int err = 0;
    iassert_locked(ip);
    if ((err = icheck_op(ip, isetattr)))
        return err;
    
    return ip->i_ops->isetattr(ip, attr);
}

int     itruncate(inode_t *ip) {
    int err = 0;
    iassert_locked(ip);

    if (IISDIR(ip))
        return -EISDIR;

    if ((err = icheck_op(ip, itruncate)))
        return err;
    
    return ip->i_ops->itruncate(ip);
}

/* check for file permission */
int check_iperm(inode_t *ip, cred_t *cred, int oflags)
{
    // printk("%s(\e[0;15mip=%p, cred=%p, oflags=%d)\e[0m\n", __func__, ip, cred, oflags);
    if (!ip || !cred)
        return -EINVAL;

    iassert_locked(ip);

    if (cred->c_uid == 0) /* root */
        return 0;

    if (((oflags & O_ACCMODE) == O_RDONLY) || (oflags & O_ACCMODE) != O_WRONLY)
    {
        if (ip->i_uid == cred->c_uid)
        {
            if (ip->i_mode & S_IRUSR)
                goto write_perms;
        }
        else if (ip->i_gid == cred->c_gid)
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
        if (ip->i_uid == cred->c_uid)
        {
            if (ip->i_mode & S_IWUSR)
                goto exec_perms;
        }
        else if (ip->i_gid == cred->c_gid)
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
        if (ip->i_uid == cred->c_uid)
        {
            if (ip->i_mode & S_IXUSR)
                goto done;
        }
        else if (ip->i_gid == cred->c_gid)
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

ssize_t iread(inode_t *ip, off_t off, void *buf, size_t sz) {
    ssize_t retval = 0;
    iassert_locked(ip);
    if (ip->i_cache == NULL)
        return iread_data(ip, off, buf, sz);

    icache_lock(ip->i_cache);    
    retval = icache_read(ip->i_cache, off, buf, sz);
    icache_unlock(ip->i_cache);
    return retval;
}

ssize_t iwrite(inode_t *ip, off_t off, void *buf, size_t sz) {
    ssize_t retval = 0;
    iassert_locked(ip);
    if (ip->i_cache == NULL)
        return iwrite_data(ip, off, buf, sz);

    icache_lock(ip->i_cache);    
    retval = icache_write(ip->i_cache, off, buf, sz);
    icache_unlock(ip->i_cache);
    return retval;
}