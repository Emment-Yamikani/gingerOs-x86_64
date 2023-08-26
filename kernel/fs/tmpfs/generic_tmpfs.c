#include <fs/generic_tmpfs.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <mm/kalloc.h>

typedef struct 
{
    char *d_name;
    inode_t *inode;
} tmpfs_dirent_t;

typedef struct {
    size_t items;
    tmpfs_dirent_t **table;
} directory_t;

static tmpfs_dirent_t *generic_tmpfs_new_dirent(void) {
    tmpfs_dirent_t *entry = NULL;
    entry = kcalloc(1, sizeof *entry);
    return entry;
}

int generic_tmpfs_create(INODE dir, const char *name, ialloc_desc_t type, INODE *ref) {
    int err = 0;
    inode_t *inode = NULL;
    directory_t *directory = NULL;
    tmpfs_dirent_t *dirent = NULL;
    tmpfs_dirent_t **table = NULL;

    if (!dir)
        return -EINVAL;

    iassert_locked(dir);

    if (!INODE_ISDIR(dir))
        return -ENOTDIR;

    if ((err = ialloc(type, &inode)))
        return err;

    directory = dir->i_priv;
    table = directory ? directory->table : NULL;

    err = -ENOMEM;
    if (!directory)
    {
        if (!(directory = kcalloc(1, sizeof *directory)))
            goto error;

        if (!(directory->table = kcalloc(2, sizeof *dirent)))
        {
            kfree(directory);
            goto error;
        }

        directory->items = 0;
        dir->i_priv = directory;
    }
    else if (!(table = krealloc((void *)table, (directory->items + 1) * sizeof(tmpfs_dirent_t))))
        goto error;
    else
        directory->table = table;

    if (!(dirent = generic_tmpfs_new_dirent()))
        goto error;

    dirent->inode = inode;
    if (!(dirent->d_name = strdup(name)))
        goto error;

    inode->i_ops = dir->i_ops;
    directory->table[directory->items++] = dirent;
    *ref = inode;

    iunlock(inode);
    return 0;
error:
    if (dirent)
        kfree(dirent);
    irelease(inode);
    return err;
}

int generic_tmpfs_itrunc(INODE inode, off_t length)
{
    void *data = NULL;

    if (!inode)
        return -EINVAL;
    
    if (INODE_ISDIR(inode))
        return -EISDIR;

    if ((ssize_t)length < 0)
        return -EINVAL;
    
    if (inode->i_size != length) {
        data = kcalloc(1, length);

        if (!data)
            return -ENOMEM;

        if (inode->i_priv) {
            memcpy(data, inode->i_priv, length);
            kfree(inode->i_priv);
        }

        inode->i_priv = data;
    }

    inode->i_size = length;

    return 0;
}

int generic_tmpfs_iperm(INODE inode __unused, int mask __unused)
{
    return 0;
}

int generic_tmpfs_iopen(INODE inode __unused, struct dentry *dentry __unused)
{
    return -EINVAL;
}

int generic_tmpfs_imkdir(INODE dir, struct dentry *dentry, mode_t mode)
{
    ialloc_desc_t idesc;
    if (!dir || !dentry)
        return -EINVAL;
    if (!INODE_ISDIR(dir))
        return -ENOTDIR;

    idesc = IPROT(FS_DIR, 0, dir->i_uid, dir->i_gid, 0, 0, mode);
    return generic_tmpfs_create(dir, dentry->d_name, idesc, &dentry->d_inode);
}

int generic_tmpfs_irmdir(INODE dir __unused, struct dentry *dentry __unused)
{
    return -EINVAL;
}

int generic_tmpfs_iunlink(INODE dir __unused, struct dentry *dentry __unused)
{
    return -EINVAL;
}

int generic_tmpfs_ilookup(INODE dir __unused, struct dentry *dentry __unused)
{
    return -EINVAL;
}

int generic_tmpfs_icreate(INODE dir, struct dentry *dentry, int mode)
{
    if (!dir || !dentry)
        return -EINVAL;
    if (!INODE_ISDIR(dir))
        return -ENOTDIR;
    return generic_tmpfs_create(dir, dentry->d_name, IPROT(FS_REG, 0, dir->i_uid, dir->i_gid, 0, 0, mode), &dentry->d_inode);
}

int generic_tmpfs_imknod(INODE dir __unused, struct dentry *dentry __unused, int mode __unused, devid_t dev __unused)
{
    return -EINVAL;
}

int generic_tmpfs_isymlink(INODE dir __unused, struct dentry *dentry __unused, const char *symname __unused)
{
    return -EINVAL;
}

int generic_tmpfs_ilink(struct dentry *old_dentry __unused, INODE dir __unused, struct dentry *new_dentry __unused)
{
    return -EINVAL;
}

int generic_tmpfs_irename(INODE old_dir __unused, struct dentry *old_dentry __unused, INODE inode __unused, struct dentry *new_dentry __unused)
{
    return -EINVAL;
}

iops_t generic_tmpfs_iops = (iops_t){
    .iperm = generic_tmpfs_iperm,
    .ilink = generic_tmpfs_ilink,
    .itrunc = generic_tmpfs_itrunc,
    .imkdir = generic_tmpfs_imkdir,
    .irmdir = generic_tmpfs_irmdir,
    .imknod = generic_tmpfs_imknod,
    .iunlink = generic_tmpfs_iunlink,
    .ilookup = generic_tmpfs_ilookup,
    .icreate = generic_tmpfs_icreate,
    .irename = generic_tmpfs_irename,
    .isymlink = generic_tmpfs_isymlink,
};