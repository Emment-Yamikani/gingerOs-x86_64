#include <fs/fs.h>
#include <bits/errno.h>
#include <mm/kalloc.h>
#include <lib/printk.h>
#include <lib/string.h>

static LIST_HEAD(icache_list);
spinlock_t *icache_list_lock = &SPINLOCK_INIT();

void idup(INODE ip)
{
    iassert_locked(ip);
    ip->i_count++;
}

int iopen(INODE ip, dentry_t *dentry)
{
    if (!ip || !dentry)
        return -EINVAL;
    iassert_locked(ip);
    dentry_assert_locked(dentry);
    idup(ip);
    list_add(&dentry->d_alias, &ip->i_dentry);
    dentry->d_inode = ip;
    return 0;
}

static void ifree(INODE ip)
{
    iassert_locked(ip);
    list_del(&ip->i_inodes);
    *ip = (inode_t){0};
    kfree(ip);
    printk("%s:%d %s() returning...\n", __FILE__, __LINE__, __func__);
}

void irelease(INODE ip)
{
    iassert_locked(ip);
    ip->i_count--;
    if (ip->i_count <= 0)
        ifree(ip);
}

int iclose(INODE ip)
{
    if (!ip) return -EINVAL;
    iassert_locked(ip);
    irelease(ip);
    return 0;
}

int ialloc(ialloc_desc_t desc, INODE *pip)
{
    int err = -ENOMEM;
    INODE ip = NULL;

    if (!pip || !desc.p_mode || !desc.p_type)
        return -EINVAL;

    if ((ip = kcalloc(1, sizeof *ip)) == NULL)
        goto error;

    ip->i_count = 0;
    ip->i_gid   = desc.p_gid;
    ip->i_uid   = desc.p_uid;
    ip->i_mode  = desc.p_mode;
    ip->i_type  = desc.p_type; 
    ip->i_flags = desc.p_flags;
    ip->i_lock = SPINLOCK_INIT();

    ilock(ip);
    INIT_LIST_HEAD(&ip->i_dentry);
    INIT_LIST_HEAD(&ip->i_inodes);

    spin_lock(icache_list_lock);
    list_add(&ip->i_inodes, &icache_list);
    spin_unlock(icache_list_lock);

    *pip = ip;
    return 0;
error:
    if (ip) kfree(ip);
    return err;
}

int itrunc(INODE inode, off_t length)
{
    int err = 0;
    iassert_locked(inode);
    
    if ((err = check_iop(inode, itrunc)))
        goto error;

    inode->i_ops->itrunc(inode, length);
    return 0;
error:
    return err;
}

int iperm(INODE inode, int mask)
{
    int err = 0;
    iassert_locked(inode);

    if ((err = check_iop(inode, iperm)))
        goto error;
    
    if ((err = inode->i_ops->iperm(inode, mask)))
        goto error;

    return 0;
error:
    return err;
}

int imkdir(INODE dir, struct dentry *dentry, mode_t mode)
{
    int err = 0;
    iassert_locked(dir);
    dentry_assert_locked(dentry);

    if (!dentry)
        return -EINVAL;

    if ((err = check_iop(dir, imkdir)))
        goto error;
    
    if ((err = dir->i_ops->imkdir(dir, dentry, mode)))
        goto error;

    return 0;
error:
    return err;
}

int irmdir(INODE dir, dentry_t *dentry)
{
    int err = 0;
    iassert_locked(dir);
    dentry_assert_locked(dentry);

    if (!dentry)
        return -EINVAL;

    if ((err = check_iop(dir, irmdir)))
        goto error;
    
    if ((err = dir->i_ops->irmdir(dir, dentry)))
        goto error;

    return 0;
error:
    return err;
}

int iunlink(INODE dir, dentry_t *dentry)
{
    int err = 0;
    iassert_locked(dir);
    dentry_assert_locked(dentry);

    if (!dentry)
        return -EINVAL;
    
    if ((err = check_iop(dir, iunlink)))
        goto error;
    
    if ((err = dir->i_ops->iunlink(dir, dentry)))
        goto error;

    return 0;
error:
    return err;
}

int ilookup(INODE dir, dentry_t *dentry)
{
    int err = 0;
    iassert_locked(dir);
    dentry_assert_locked(dentry);

    if (!dentry)
        return -EINVAL;

    if ((err = check_iop(dir, ilookup)))
        goto error;
    
    if ((err = dir->i_ops->ilookup(dir, dentry)))
        goto error;

    return 0;
error:
    return err;
}

int icreate(INODE dir, dentry_t *dentry, int mode)
{
    int err = 0;
    iassert_locked(dir);
    dentry_assert_locked(dentry);

    if (!dentry)
        return -EINVAL;

    if ((err = check_iop(dir, icreate)))
        goto error;
    
    if ((err = dir->i_ops->icreate(dir, dentry, mode)))
        goto error;

    return 0;
error:
    return err;
}

int imknod(INODE dir, dentry_t *dentry, int mode, devid_t dev)
{
    int err = 0;
    iassert_locked(dir);
    dentry_assert_locked(dentry);

    if (!dentry)
        return -EINVAL;

    if ((err = check_iop(dir, imknod)))
        goto error;
    
    if ((err = dir->i_ops->imknod(dir, dentry, mode, dev)))
        goto error;

    return 0;
error:
    return err;
}

int isymlink(INODE dir, dentry_t *dentry, const char *symname)
{
    int err = 0;
    iassert_locked(dir);
    dentry_assert_locked(dentry);

    if (!dentry || !symname)
        return -EINVAL;

    if ((err = check_iop(dir, isymlink)))
        goto error;
    
    if ((err = dir->i_ops->isymlink(dir, dentry, symname)))
        goto error;

    return 0;
error:
    return err;
}

int ilink(dentry_t *old_dentry, INODE dir, dentry_t *new_dentry)
{
    int err = 0;
    iassert_locked(dir);
    dentry_assert_locked(old_dentry);
    dentry_assert_locked(new_dentry);

    if (!old_dentry || !new_dentry)
        return -EINVAL;

    if ((err = check_iop(dir, ilink)))
        goto error;
    
    if ((err = dir->i_ops->ilink(old_dentry, dir, new_dentry)))
        goto error;

    return 0;
error:
    return err;
}

int irename(INODE old_dir, dentry_t *old_dentry, INODE new_dir, dentry_t *new_dentry)
{
    int err = 0;
    iassert_locked(old_dir);
    iassert_locked(new_dir);
    dentry_assert_locked(old_dentry);
    dentry_assert_locked(new_dentry);

    if (!old_dir || !new_dir || !old_dentry || !new_dentry)
        return -EINVAL;
    
    if ((err = check_iop(old_dir, irename)))
        goto error;
    
    if ((err = old_dir->i_ops->irename(old_dir, old_dentry, new_dir, new_dentry)))
        goto error;

    return 0;
error:
    return err;
}
