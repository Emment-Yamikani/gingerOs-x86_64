#include <fs/fs.h>
#include <fs/inode.h>
#include <fs/dentry.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <lib/types.h>
#include <mm/kalloc.h>
#include <sys/_fcntl.h>
#include <sys/_stat.h>

int ialloc(inode_t **pip) {
    int err = -ENOMEM;
    inode_t *ip = NULL;

    if (pip == NULL)
        return -EINVAL;

    if ((ip = kmalloc(sizeof *ip)) == NULL)
        return err;

    memset(ip, 0, sizeof *ip);

    ip->i_refcnt = 1;
    ip->i_lock = SPINLOCK_INIT();
    ilock(ip);
    *pip = ip;

    return 0;
}

void ifree(inode_t *ip) {
    iassert_locked(ip);

    if (ip->i_refcnt <= 0) {
        iunlink(ip);
        kfree(ip);
    }
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
        /// Remember we locked this node in to forlink() loop above?,
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

int     ilink(struct dentry *oldname, inode_t *dir, struct dentry *newname) {
    int err = 0;
    iassert_locked(dir);
    dassert_locked(oldname);
    dassert_locked(newname);

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

ssize_t iread(inode_t *ip, off_t off, void *buf, size_t nb) {
    ssize_t err = 0;

    iassert_locked(ip);

    if (IISDIR(ip))
        return -EISDIR;

    if ((err = icheck_op(ip, iread)))
        return err;
    
    return ip->i_ops->iread(ip, off, buf, nb);
}

ssize_t iwrite(inode_t *ip, off_t off, void *buf, size_t nb) {
    ssize_t err = 0;
    iassert_locked(ip);

    if (IISDIR(ip))
        return -EISDIR;

    if ((err = icheck_op(ip, iwrite)))
        return err;
    
    return ip->i_ops->iwrite(ip, off, buf, nb);
}

int     imknod(inode_t *dir, struct dentry *dentry, mode_t mode, int devid) {
    int err = 0;
    iassert_locked(dir);
    dassert_locked(dentry);

    if (IISDIR(dir) == 0)
        return -ENOTDIR;

    if ((err = icheck_op(dir, imknod)))
        return err;
    
    return dir->i_ops->imknod(dir, dentry, mode, devid);
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

int     irename(inode_t *dir, struct dentry *old, inode_t *newdir, struct dentry *new) {
    int err = 0;
    iassert_locked(dir);
    iassert_locked(newdir);
    dassert_locked(old);
    dassert_locked(new);

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