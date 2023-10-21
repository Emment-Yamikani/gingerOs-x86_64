#include <fs/fs.h>
#include <bits/errno.h>
#include <ds/btree.h>
#include <mm/kalloc.h>
#include <fs/tmpfs.h>
#include <ds/hash.h>

typedef struct tmpfs_inode_t {
    uid_t       uid;
    gid_t       gid;
    mode_t      mode;
    uintptr_t   ino;
    itype_t     type;
    size_t      size;
    long        hlink;
    void        *data;
} tmpfs_inode_t;

typedef struct tmpfs_dirent_t {
    char                    *name;
    tmpfs_inode_t           *inode;
} tmpfs_dirent_t;

#define tmpfs_data(ip) ({       \
    void *data = NULL;          \
    tmpfs_inode_t *tino = NULL; \
    if ((tino = ip->i_priv))    \
        data = tino->data;      \
    data;                       \
})

static filesystem_t *tmpfs = NULL;

int tmpfs_init(void);
static size_t tmpfs_hash(const char *str);
static int tmpfs_new_inode(itype_t type, inode_t **pip);
static int tmpfs_ialloc(itype_t type, tmpfs_inode_t **pip);
static int tmpfs_hash_verify(const char *fname, tmpfs_dirent_t *dirent);

static hash_ctx_t tmpfs_hash_ctx = {
    .hash_func = tmpfs_hash,
    .hash_verify_obj = tmpfs_hash_verify,
};

static iops_t tmpfs_iops = {
    .imkdir = tmpfs_imkdir,
    .icreate = tmpfs_icreate,
    .ilookup = tmpfs_ilookup,
};

static size_t tmpfs_hash(const char *str) {
    int c = 0;
    size_t hash = 5381;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

static int tmpfs_hash_verify( const char *fname, tmpfs_dirent_t *dirent) {
    if (dirent == NULL || fname == NULL)
        return -EINVAL;
    return compare_strings(fname, dirent->name);
}

static int tmpfs_fill_sb(filesystem_t *fs __unused, const char *target,
    struct devid *devid __unused, superblock_t *sb) {
    int err = 0;
    inode_t *iroot = NULL;
    dentry_t *droot = NULL;

    if ((err = tmpfs_new_inode(FS_DIR, &iroot)))
        return err;
    
    if ((err = dalloc(target, &droot))) {
        irelease(iroot);
        return err;
    }
    
    if ((err = iadd_alias(iroot, droot))) {
        dclose(droot);
        irelease(iroot);
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

static int tmpfs_getsb(filesystem_t *fs, const char *src __unused, const char *target,
                        unsigned long flags, void *data, superblock_t **psbp) {
    return getsb_nodev(fs, target, flags, data, psbp, tmpfs_fill_sb);
}

int tmpfs_init(void) {
    int err = 0;

    if ((err = fs_create("tmpfs", &tmpfs_iops, &tmpfs)))
        return err;

    tmpfs->get_sb = tmpfs_getsb;
    tmpfs->mount = NULL;

    if ((err = vfs_register_fs(tmpfs))) {
        fsunlock(tmpfs);
        goto error;
    }

    fsunlock(tmpfs);
    return 0;
error:
    if (tmpfs)
        fs_free(tmpfs);
    return err;
}

static int tmpfs_ialloc(itype_t type, tmpfs_inode_t **pipp) {
    int err = 0;
    tmpfs_inode_t *ip = NULL;
    static long tmpfs_inos = 0;
    hash_table_t *htable = NULL;

    if (pipp == NULL)
        return -EINVAL;
    
    if ((ip = kmalloc(sizeof *ip)) == NULL)
        return -ENOMEM;

    if (type == FS_DIR) {
        if ((err = hash_alloc(&tmpfs_hash_ctx, &htable)))
            goto error;
    }
    
    memset(ip, 0, sizeof *ip);

    ip->hlink   = 1;
    ip->type    = type;
    ip->data    = htable;
    ip->ino     = atomic_fetch_inc(&tmpfs_inos);

    *pipp = ip;

    return 0;
error:
    if (ip)
        kfree(ip);
    if (htable)
        hash_destroy(htable);
    return err;
}

static int tmpfs_ifree(tmpfs_inode_t *ip) {
    int err = 0;
    hash_table_t *htable = NULL;

    if (ip == NULL)
        return -EINVAL;

    ip->hlink--;

    if (ip->hlink <= 0) {
        if (ip->type == FS_DIR) {
            if ((htable = ip->data) == NULL)
                return -EINVAL;

            hash_lock(htable);
            if ((err = hash_free(htable))) {
                hash_unlock(htable);
                ip->hlink++;
                goto error;
            }
            hash_unlock(htable);
        }
        else if (ip->data)
            kfree(ip->data);
    }

    return 0;
error:
    return err;
}

static int tmpfs_new_inode(itype_t type, inode_t **pip) {
    int err =  0;
    char *name = NULL;
    inode_t *inode = NULL;
    tmpfs_inode_t *tmpfs_ip = NULL;

    if (pip == NULL)
        return -EINVAL;

    if ((err = tmpfs_ialloc(type, &tmpfs_ip)))
        goto error;

    if ((err = ialloc(&inode)))
        goto error;

    inode->i_type   = type;
    inode->i_priv   = tmpfs_ip;
    inode->i_ops    = &tmpfs_iops;
    inode->i_ino    = tmpfs_ip->ino;

    *pip = inode;
    return 0;
error:
    if (name)
        kfree(name);

    if (tmpfs_ip)
        tmpfs_ifree(tmpfs_ip);
    return err;
}

static int tmpfs_dirent_alloc(const char *fname, tmpfs_inode_t *ip, tmpfs_dirent_t **ptde) {
    int err = 0;
    char *name = NULL;
    tmpfs_dirent_t *tde = NULL;

    if (fname == NULL || ptde == NULL)
        return -EINVAL;

    if ((name = strdup(fname)) == NULL)
        return -ENOMEM;

    err = -ENOMEM;
    if ((tde = kmalloc(sizeof *tde)) == NULL)
        goto error;

    tde->name = name;
    tde->inode = ip;

    *ptde = tde;
    return 0;
error:
    if (name)
        kfree(name);
    
    if (tde)
        kfree(tde);
    
    return err;
}

static int tmpfs_dirent_free(tmpfs_dirent_t *dirent) {
    if (dirent == NULL)
        return -EINVAL;
    
    if (dirent->inode)
        dirent->inode->hlink--;

    if (dirent->name)
        kfree(dirent->name);

    kfree(dirent);
    return 0;
}

static int tmpfs_create_node(inode_t *dir, const char *fname, mode_t mode, itype_t type) {
    int err = 0;
    tmpfs_inode_t *ip = NULL;
    hash_table_t *htable = NULL;
    tmpfs_dirent_t *dirent = NULL;

    iassert_locked(dir);

    if (IISDIR(dir) == 0)
        return -ENOTDIR;

   if ((htable = tmpfs_data(dir)) == NULL)
        return -EINVAL;

    if ((err = tmpfs_ialloc(type, &ip)))
        return err;
    
    if ((err = tmpfs_dirent_alloc(fname, ip, &dirent)))
        goto error;

    hash_lock(htable);
    if ((err = hash_insert(htable, (void *)fname, dirent))) {
        hash_unlock(htable);
        goto error;
    }
    hash_unlock(htable);

    ip->mode = mode;

    printk("tmpfs created file \"%s\"\n", fname);
    printk("[\e[0;04mWARNING\e[0m]: file credentials not fully set!\n");
    return 0;
error:
    if (dirent)
        tmpfs_dirent_free(dirent);
    if (ip) {
        err = tmpfs_ifree(ip);
    }
    return err;
}

int tmpfs_imkdir(inode_t *dir, const char *fname, mode_t mode) {
    return tmpfs_create_node(dir, fname, mode, FS_DIR);
}

int tmpfs_icreate(inode_t *dir, const char *fname, mode_t mode) {
    return tmpfs_create_node(dir, fname, mode, FS_RGL);
}

int tmpfs_ilookup(inode_t *dir, const char *fname, inode_t **pipp) {
    int err = 0;
    inode_t *ip = NULL;
    hash_table_t *htable = NULL;
    tmpfs_dirent_t *dirent = NULL;

    iassert_locked(dir);

    if (dir == NULL || pipp == NULL)
        return -EINVAL;
    
    if (IISDIR(dir) == 0)
        return -ENOTDIR;

    if (NULL == (htable = tmpfs_data(dir)))
        return -EINVAL;

    printk("tmpfs looking-up file(\e[0;013m%s\e[0m).\n", fname);
 
    hash_lock(htable);
    if ((err = hash_search(htable, (void *)fname, -1, (void **)&dirent))) {
        hash_unlock(htable);
        goto error;
    }
    hash_unlock(htable);

    printk("tmpfs found file(\e[0;013m%s\e[0m).\n", fname);

    if (dirent->inode == NULL)
        return -EINVAL;
    
    if ((err = ialloc(&ip)))
        goto error;

    ip->i_ops       = &tmpfs_iops;
    ip->i_priv      = dirent->inode;
    ip->i_ino       = dirent->inode->ino;
    ip->i_type      = dirent->inode->type;
    ip->i_size      = dirent->inode->size;
    ip->i_hlinks    = dirent->inode->hlink;
    ip->i_gid       = dirent->inode->gid;
    ip->i_uid       = dirent->inode->uid;
    ip->i_mode      = dirent->inode->mode;

    *pipp = ip;    
    return 0;
error:
    if (ip)
        irelease(ip);

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
int tmpfs_ibind(inode_t *dir, struct dentry *dentry, inode_t *ip);
ssize_t tmpfs_iread(inode_t *ip, off_t off, void *buf, size_t nb);
ssize_t tmpfs_iwrite(inode_t *ip, off_t off, void *buf, size_t nb);
int tmpfs_isymlink(inode_t *ip, inode_t *atdir, const char *symname);
ssize_t tmpfs_ireaddir(inode_t *dir, off_t off, void *buf, size_t count);
int tmpfs_ilink(struct dentry *oldname, inode_t *dir, struct dentry *newname);
int tmpfs_imknod(inode_t *dir, struct dentry *dentry, mode_t mode, int devid);
int tmpfs_irename(inode_t *dir, struct dentry *old, inode_t *newdir, struct dentry *new);