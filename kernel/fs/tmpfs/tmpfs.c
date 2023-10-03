#include <fs/fs.h>
#include <bits/errno.h>
#include <ds/btree.h>
#include <mm/kalloc.h>
#include <fs/tmpfs.h>

typedef struct {
    uio_t       uio;
    uintptr_t   ino;
    itype_t     type;
    size_t      size;
    size_t      nlink;
    void        *data;
    void        *priv;
} tmpfs_inode_t;

typedef struct tmpfs_dirent {
    struct tmpfs_dirent *prev;
    tmpfs_inode_t       *d_ino;
    size_t              d_len;
    struct tmpfs_dirent *next;
    char                d_name[];
} tmpfs_dirent_t;

static filesystem_t *tmpfs = NULL;


int tmpfs_init(void);
static int tmpfs_ialloc(itype_t type, inode_t **pip);

static iops_t tmpfs_iops = {
    .imkdir = tmpfs_imkdir,
    .icreate = tmpfs_icreate,

};

static size_t tmpfs_hash(const char *str) {
    int c = 0;
    size_t hash = 5381;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

static int tmpfs_fill_sb(filesystem_t *fs __unused, struct devid *devid __unused, superblock_t *sb) {
    int err = 0;
    inode_t *iroot = NULL;
    dentry_t *droot = NULL;

    printk("%s()...\n", __func__);

    if ((err = tmpfs_ialloc(FS_DIR, &iroot)))
        return err;
    
    if ((err = dalloc("/", &droot))) {
        iclose(iroot);
        return err;
    }
    
    if ((err = iadd_alias(iroot, droot))) {
        dclose(droot);
        iclose(iroot);
        return err;
    }

    sb->sb_blocksize = -1;
    strncpy(sb->sb_magic0, "virtual FS", 10);
    sb->sb_size = -1;
    sb->sb_uio = (uio_t){
        .u_cwd = "/",
        .u_root = "/",
        .u_gid = 0,
        .u_uid = 0,
        .u_umask = 0555,
    };
    sb->sb_root = droot;

    iroot->i_sb = sb;
    iroot->i_type = FS_DIR;
    iroot->i_ops = sb->sb_iops;
    dunlock(droot);
    iunlock(iroot);
    return 0;
}

static int tmpfs_getsb(filesystem_t *fs, const char *src __unused, unsigned long flags, void *data, superblock_t **psbp) {
    printk("%s()...\n", __func__);
    return getsb_nodev(fs, flags, data, psbp, tmpfs_fill_sb);
}

int tmpfs_init(void) {
    int err = 0;

    if ((err = fs_create("tmpfs", &tmpfs_iops, &tmpfs)))
        return err;

    tmpfs->get_sb = tmpfs_getsb;
    tmpfs->mount = NULL;

    if ((err = vfs_register_fs(tmpfs)))
        goto error;

    fsunlock(tmpfs);
    return 0;
error:
    if (tmpfs)
        fs_free(tmpfs);
    return err;
}

static int tmpfs_ialloc(itype_t type, inode_t **pip) {
    int err =  0;
    btree_t *bt = NULL;
    inode_t *inode = NULL;
    static long tmpfs_inos = 0;
    tmpfs_inode_t *tmpfs_ip = NULL;

    if (pip == NULL)
        return -EINVAL;

    if ((tmpfs_ip = kmalloc(sizeof *tmpfs_ip)) == NULL)
        return ENOMEM;

    if (type == FS_DIR) {
        if ((err = btree_alloc(&bt)))
            goto error;
    }

    memset(tmpfs_ip, 0, sizeof *tmpfs_ip);

    if ((err = ialloc(&inode)))
        goto error;

    inode->i_type = type;
    inode->i_priv = tmpfs_ip;
    inode->i_ops = &tmpfs_iops;

    tmpfs_ip->data = bt;
    tmpfs_ip->type= type;
    tmpfs_ip->priv = inode;
    tmpfs_ip->ino = atomic_fetch_inc(&tmpfs_inos);
    inode->i_ino = tmpfs_ip->ino;

    *pip = inode;
    return 0;
error:
    if (tmpfs_ip)
        kfree(tmpfs_ip);
    
    if (bt)
        btree_free(bt);
    return err;
}

static int tmpfs_search_dir(inode_t *dir, const char *fn, tmpfs_dirent_t **pdp) {
    int err = 0;
    size_t hash = 0;
    btree_t *bt = NULL;
    tmpfs_inode_t *tdp = NULL;
    tmpfs_dirent_t *dirent = NULL, *next = NULL;

    if (dir == NULL || pdp == NULL)
        return -EINVAL;
    
    if (fn == NULL || *fn == '\0')
        return -ENOTNAM;
    
    iassert_locked(dir);

    if ((IISDIR(dir)) == 0)
        return -ENOTDIR;

    if ((tdp = dir->i_priv) == NULL)
        return -EINVAL;
    
    if ((bt = tdp->data) == NULL)
        return -EINVAL;
    
    hash = tmpfs_hash(fn);

    btree_lock(bt);
    err = btree_search(bt, hash, (void **)&dirent);
    btree_unlock(bt);

    if (err)
        return err;

    if (compare_strings(fn, dirent->d_name)) {
        forlinked(node, dirent->next, next) {
            next = node->next;
            if (!compare_strings(fn, node->d_name)) {
                dirent = node;
                goto done;
            }
        }
        return -ENOENT;
    }

done:
    *pdp = dirent;
    return 0;
}

static int tmpfs_insert(inode_t *dir, tmpfs_dirent_t *dirent) {
    int err = 0;
    size_t hash = 0;
    btree_t *bt = NULL;
    tmpfs_inode_t *pti = NULL;
    tmpfs_dirent_t *pde = NULL, *last = NULL;

    if (dir == NULL || dirent == NULL)
        return -EINVAL;
    
    iassert_locked(dir);

    if (IISDIR(dir) == 0)
        return -ENOTDIR;

    if ((pti = dir->i_priv) == NULL)
        return -EINVAL;
    
    if ((bt = pti->data) == NULL)
        return -EINVAL;
    
    if (!(err = tmpfs_search_dir(dir, dirent->d_name, &pde)))
        return -EEXIST;

    hash = tmpfs_hash(dirent->d_name);

    btree_lock(bt);
    if (!(err = btree_search(bt, hash, (void **)&pde))) {
        forlinked(node, pde, node->next)
            last = node;
        last->next = dirent;
        dirent->prev = last;
        dirent->next = NULL;
    } else
        err = btree_insert(bt, hash, dirent);
    btree_unlock(bt);
    return err;
}

__unused static int tmpfs_delete(inode_t *dir, const char *fn) {
    int err = 0;
    size_t hash = 0;
    btree_t *bt = NULL;
    tmpfs_dirent_t *pde = NULL, *dirent = NULL;

    if ((err = tmpfs_search_dir(dir, fn, &pde)))
        return err;

    if (pde->d_ino == NULL)
        return -EINVAL;
    
    if ((bt = pde->d_ino->data) == NULL)
        return -EINVAL;
    
    hash = tmpfs_hash(fn);

    btree_lock(bt);
    if (!(err = btree_search(bt, hash, (void **)&dirent))) {
        if ((pde == dirent)) {
            btree_delete(bt, hash);
            if (pde->next)
                err = tmpfs_insert(dir, pde->next);
        } else {
            if (pde->prev)
                pde->prev->next = pde->next;
            if (pde->next)
                pde->next->prev = pde->prev;
        }
    }
    btree_unlock(bt);

    return 0;
}

int tmpfs_imkdir(inode_t *dir, struct dentry *dentry, mode_t mode) {
    int err = 0;
    inode_t *ip = NULL;
    btree_t *bt = NULL;
    tmpfs_dirent_t *dp = NULL;

    iassert_locked(dir);
    dassert_locked(dentry);

    printk("[WARNING]:: check permissions: mode(%x)\n", mode);

    if ((dp = kmalloc(sizeof *dp + strlen(dentry->d_name) + 1)) == NULL)
        return -ENOMEM;

    dp->d_len = strlen(dentry->d_name);

    if ((err = tmpfs_ialloc(FS_DIR, &ip)))
        return err;

    if ((err = iadd_alias(ip, dentry)))
        goto error;
    
    ip->i_mode = mode;

    bt = dir->i_priv ? ((tmpfs_inode_t *)dir->i_priv)->data : NULL;

    err = -EINVAL;
    if (bt == NULL)
        goto error;

    dp->d_ino = ip->i_priv;
    dp->d_name[dp->d_len] = '\0';
    strncmp(dp->d_name, dentry->d_name, dp->d_len);

    if ((err = tmpfs_insert(dir, dp)))
        goto error;

    iunlock(ip);
    return 0;
error:
    if (dp) kfree(dp);
    if (ip) iclose(ip);
    return err;
}

int tmpfs_icreate(inode_t *dir, struct dentry *dentry, mode_t mode) {
    int err = 0;
    inode_t *ip = NULL;
    btree_t *bt = NULL;
    tmpfs_dirent_t *dp = NULL;

    iassert_locked(dir);
    dassert_locked(dentry);

    printk("[WARNING]:: check permissions: mode(%x)\n", mode);

    if ((dp = kmalloc(sizeof *dp + strlen(dentry->d_name) + 1)) == NULL)
        return -ENOMEM;

    dp->d_len = strlen(dentry->d_name);

    if ((err = tmpfs_ialloc(FS_RGL, &ip)))
        return err;

    if ((err = iadd_alias(ip, dentry)))
        goto error;
    
    ip->i_mode = mode;

    bt = dir->i_priv ? ((tmpfs_inode_t *)dir->i_priv)->data : NULL;

    err = -EINVAL;
    if (bt == NULL)
        goto error;

    dp->d_ino = ip->i_priv;
    dp->d_name[dp->d_len] = '\0';
    strncmp(dp->d_name, dentry->d_name, dp->d_len);

    if ((err = tmpfs_insert(dir, dp)))
        goto error;

    iunlock(ip);
    return 0;
error:
    if (dp) kfree(dp);
    if (ip) iclose(ip);
    return err;
}

int tmpfs_isync(inode_t *ip);
int tmpfs_iclose(inode_t *ip);
int tmpfs_iunlink(inode_t *ip);
int tmpfs_itruncate(inode_t *ip);
int tmpfs_igetattr(inode_t *ip, void *attr);
int tmpfs_isetattr(inode_t *ip, void *attr);
int tmpfs_ifcntl(inode_t *ip, int cmd, void *argp);
int tmpfs_iioctl(inode_t *ip, int req, void *argp);
int tmpfs_ilookup(inode_t *dir, struct dentry *dentry);
int tmpfs_ibind(inode_t *dir, struct dentry *dentry, inode_t *ip);
ssize_t tmpfs_iread(inode_t *ip, off_t off, void *buf, size_t nb);
ssize_t tmpfs_iwrite(inode_t *ip, off_t off, void *buf, size_t nb);
int tmpfs_isymlink(inode_t *ip, inode_t *atdir, const char *symname);
ssize_t tmpfs_ireaddir(inode_t *dir, off_t off, void *buf, size_t count);
int tmpfs_ilink(struct dentry *oldname, inode_t *dir, struct dentry *newname);
int tmpfs_imknod(inode_t *dir, struct dentry *dentry, mode_t mode, int devid);
int tmpfs_irename(inode_t *dir, struct dentry *old, inode_t *newdir, struct dentry *new);