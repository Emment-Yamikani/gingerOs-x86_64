#include <fs/fs.h>
#include <bits/errno.h>
#include <ds/btree.h>
#include <mm/kalloc.h>
#include <fs/tmpfs.h>
#include <ds/hash.h>
#include <ds/stack.h>
#include <fs/stat.h>
#include <fs/devtmpfs.h>
#include <dev/dev.h>

typedef struct tmpfs_inode_t {
    uid_t       uid;
    gid_t       gid;
    mode_t      mode;
    uintptr_t   ino;
    itype_t     type;
    devid_t     rdev;
    size_t      size;
    long        hlink;
    void        *data;
} tmpfs_inode_t;

typedef struct tmpfs_dirent_t {
    char                    *name;
    tmpfs_inode_t           *inode;
} tmpfs_dirent_t;

int tmpfs_init(void);
static size_t tmpfs_hash(const char *str);
int tmpfs_new_inode(itype_t type, inode_t **pip);
static int tmpfs_ialloc(itype_t type, tmpfs_inode_t **pip);
static int tmpfs_hash_verify(const char *fname, tmpfs_dirent_t *dirent);

#define tmpfs_data(ip) ({       \
    void *data = NULL;          \
    tmpfs_inode_t *tino = NULL; \
    if ((tino = ip->i_priv))    \
        data = tino->data;      \
    data;                       \
})

static filesystem_t *tmpfs = NULL;

static hash_ctx_t tmpfs_hash_ctx = {
    .hash_func = tmpfs_hash,
    .hash_verify_obj = tmpfs_hash_verify,
};

static iops_t tmpfs_iops = {
    .ibind      = tmpfs_ibind,
    .isync      = tmpfs_isync,
    .ilink      = tmpfs_ilink,
    .iread      = tmpfs_iread,
    .iwrite     = tmpfs_iwrite,
    .iclose     = tmpfs_iclose,
    .ifcntl     = tmpfs_ifcntl,
    .iioctl     = tmpfs_iioctl,
    .imkdir     = tmpfs_imkdir,
    .imknod     = tmpfs_imknod,
    .icreate    = tmpfs_icreate,
    .ilookup    = tmpfs_ilookup,
    .iunlink    = tmpfs_iunlink,
    .irename    = tmpfs_irename,
    .igetattr   = tmpfs_igetattr,
    .isetattr   = tmpfs_isetattr,
    .isymlink   = tmpfs_isymlink,
    .ireaddir   = tmpfs_ireaddir,
    .itruncate  = tmpfs_itruncate,
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
    sb->sb_uio = (cred_t){
        .c_gid  = 0,
        .c_uid  = 0,
        .c_umask= 0555,
        .c_lock = SPINLOCK_INIT(),
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
    int             err         = 0;
    tmpfs_inode_t   *ip         = NULL;
    hash_table_t    *htable     = NULL;
    static long     tmpfs_inos  = 0;

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
        }
        else if (ip->data)
            kfree(ip->data);
    }

    return 0;
error:
    return err;
}

int tmpfs_new_inode(itype_t type, inode_t **pip) {
    int err =  0;
    char *name = NULL;
    inode_t *inode = NULL;
    tmpfs_inode_t *tmpfs_ip = NULL;

    if (pip == NULL)
        return -EINVAL;

    if ((err = tmpfs_ialloc(type, &tmpfs_ip)))
        goto error;

    if ((err = ialloc(type, 0, &inode)))
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
    int             err     = 0;
    tmpfs_dirent_t  *tde    = NULL;
    char            *name   = NULL;

    if (fname == NULL || ptde == NULL)
        return -EINVAL;

    if ((name = strdup(fname)) == NULL)
        return -ENOMEM;

    err = -ENOMEM;
    if ((tde = kmalloc(sizeof *tde)) == NULL)
        goto error;

    tde->inode  = ip;
    tde->name   = name;

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

static int tmpfs_create_node(inode_t *dir, const char *fname, mode_t mode, itype_t type, devid_t dev __unused) {
    int             err     = 0;
    tmpfs_inode_t   *ip     = NULL;
    hash_table_t    *htable = NULL;
    tmpfs_dirent_t  *dirent = NULL;

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
    ip->rdev = dev;

    // printk("%s:%d: [\e[0;04mWARNING\e[0m]: file credentials not fully set!: rdev[%d:%d]\n",
        //    __FILE__, __LINE__, dev & 0xff, dev >> 8);
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
    return tmpfs_create_node(dir, fname, mode, FS_DIR, 0);
}

int tmpfs_icreate(inode_t *dir, const char *fname, mode_t mode) {
    return tmpfs_create_node(dir, fname, mode, FS_RGL, 0);
}

int tmpfs_ilookup(inode_t *dir, const char *fname, inode_t **pipp) {
    int             err     = 0;
    inode_t         *ip     = NULL;
    hash_table_t    *htable = NULL;
    tmpfs_dirent_t  *dirent = NULL;

    iassert_locked(dir);

    if (dir == NULL || pipp == NULL)
        return -EINVAL;
    
    if (IISDIR(dir) == 0)
        return -ENOTDIR;

    if (NULL == (htable = tmpfs_data(dir)))
        return -EINVAL;

    // printk("tmpfs looking-up file(\e[0;013m%s\e[0m).\n", fname);
 
    hash_lock(htable);
    if ((err = hash_search(htable, (void *)fname, -1, (void **)&dirent))) {
        hash_unlock(htable);
        goto error;
    }
    hash_unlock(htable);

    // printk("tmpfs found file(\e[0;013m%s\e[0m).\n", fname);

    if (dirent->inode == NULL)
        return -EINVAL;
    
    if ((err = ialloc(dirent->inode->type, 0, &ip)))
        goto error;

    if (IISDEV(ip))
        ip->i_ops   = &dev_iops;
    else
        ip->i_ops   = dir->i_ops;

    // printk("file_type: %s\n", itype_strings[dirent->inode->type]);

    ip->i_priv      = dirent->inode;
    ip->i_ino       = dirent->inode->ino;
    ip->i_type      = dirent->inode->type;
    ip->i_size      = dirent->inode->size;
    ip->i_hlinks    = dirent->inode->hlink;
    ip->i_gid       = dirent->inode->gid;
    ip->i_uid       = dirent->inode->uid;
    ip->i_mode      = dirent->inode->mode;
    ip->i_rdev      = dirent->inode->rdev;

    // printk("%s:%d: rdev[%d:%d]\n",
        // __FILE__, __LINE__, ip->i_rdev & 0xff, ip->i_rdev >> 8);
    *pipp = ip;    
    return 0;
error:
    if (ip)
        irelease(ip);

    return err;
}

int tmpfs_itruncate(inode_t *ip) {
    void *data = NULL;
    tmpfs_inode_t *tino = NULL;

    iassert_locked(ip);
    if (ip == NULL)
        return -EINVAL;
    
    if (IISDIR(ip))
        return -EISDIR;
    
    if (IISDEV(ip))
        return -EINVAL;

    if ((tino = ip->i_priv) == NULL)
        return -EINVAL;

    if ((data = tmpfs_data(ip)) == NULL)
        return 0;
    
    kfree(data);

    ip->i_size = 0;
    tino->size = 0;
    tino->data = NULL;

    return 0;
}

ssize_t tmpfs_iread(inode_t *ip, off_t off, void *buf, size_t sz) {
    void *data = NULL;
    tmpfs_inode_t *tino = NULL;

    iassert_locked(ip);

    if (ip == NULL || buf == NULL)
        return -EINVAL;
    
    if (IISDIR(ip))
        return -EISDIR;

    data = tmpfs_data(ip);
    tino = ip->i_priv;

    if ((off >= tino->size) || data == NULL)
        return -1;
    sz = MIN((tino->size - off), sz);
    memcpy(buf, (data + off), sz);

    return sz;
}

ssize_t tmpfs_iwrite(inode_t *ip, off_t off, void *buf, size_t sz) {
    void *data = NULL;
    tmpfs_inode_t *tino = NULL;

    iassert_locked(ip);

    if (ip == NULL || buf == NULL)
        return -EINVAL;

    if (IISDIR(ip))
        return -EISDIR;

    if ((tino = ip->i_priv) == NULL)
        return -EINVAL;

    if ((off + sz) > tino->size) {
        if (tino->data == NULL) {
            if ((data = kmalloc(off + sz)) == NULL)
                return -EAGAIN;
        } else {
            if ((data = krealloc(tino->data, off + sz)) == NULL)
                return -EAGAIN;
        }

        tino->data = data;
        tino->size = off + sz;
        ip->i_size = tino->size;
    }

    sz = MIN((tino->size - off), sz);
    memcpy(data + off, buf, sz);
    return sz;
}

int tmpfs_isync(inode_t *ip __unused) {
    return -ENOTSUP;
}

int tmpfs_iclose(inode_t *ip __unused) {
    return -ENOTSUP;
}

int tmpfs_iunlink(inode_t *ip __unused) {
    return -ENOTSUP;
}

int tmpfs_igetattr(inode_t *ip __unused, void *attr __unused) {
    return -ENOTSUP;
}

int tmpfs_isetattr(inode_t *ip __unused, void *attr __unused) {
    return -ENOTSUP;
}

int tmpfs_ifcntl(inode_t *ip __unused, int cmd __unused, void *argp __unused) {
    return -ENOTSUP;
}

int tmpfs_iioctl(inode_t *ip, int req __unused, void *argp) {
    iassert_locked(ip);
    
    if (ip == NULL || argp == NULL)
        return -EINVAL;
    
    if (IISDEV(ip) == 0)
        return -ENOTTY;

    return -ENOTSUP;
}

int tmpfs_ibind(inode_t *dir __unused, dentry_t *dentry __unused, inode_t *ip __unused) {
    return -ENOTSUP;
}

int tmpfs_isymlink(inode_t *ip __unused, inode_t *atdir __unused, const char *symname __unused) {
    return -ENOTSUP;
}

ssize_t tmpfs_ireaddir(inode_t *dir, off_t off, struct dirent *buf, size_t count) {
    int err = 0;
    size_t pos = 0, ncount = 0;
    tmpfs_inode_t *tino = NULL;
    hash_table_t *htable = NULL;
    queue_t *queue = QUEUE_NEW();
    tmpfs_dirent_t *dirent = NULL;

    iassert_locked(dir);
    if (dir == NULL || buf == NULL)
        return -EINVAL;
    
    if (IISDIR(dir) == 0)
        return -ENOTDIR;
    
    if ((tino = dir->i_priv) == NULL)
        return -EINVAL;
    
    if ((htable = tmpfs_data(dir)) == NULL)
        return -EINVAL;
    
    if (count == 0)
        return -EINVAL;
    
    hash_lock(htable);
    queue_lock(queue);
    
    if ((err = hash_traverse(htable, queue)) == 0) {
        if (off >= queue_count(queue)) {
            err = -1;
            goto done;
        }
        forlinked(node, queue->head, node->next) {
            if (ncount >= count)
                break;
            if (pos < off) {
                pos++;
                continue;
            }
            dirent = node->data;
            buf[ncount] = (struct dirent) {
                .d_ino = dirent->inode->ino,
                .d_size = dirent->inode->size,
                .d_off = off + ncount,
                .d_reclen = sizeof *dirent,
                .d_type = (int[]) {
                    [FS_INV] = 0,
                    [FS_RGL] = _IFREG,
                    [FS_DIR] = _IFDIR,
                    [FS_BLK] = _IFBLK,
                    [FS_CHR] = _IFCHR,
                    [FS_FIFO]= _IFIFO,
                }[dirent->inode->type],
                .d_name[0] = '\0',
            };

            buf[ncount].d_type = dirent->inode->type;
            strncpy(buf[ncount].d_name, dirent->name, strlen(dirent->name));
            ncount++;
        }
    }

done:
    queue_flush(queue);
    queue_unlock(queue);
    hash_unlock(htable);

    return err;
}

int tmpfs_ilink(const char *oldname __unused, inode_t *dir __unused, const char *newname __unused) {
    return -ENOTSUP;
}

int tmpfs_imknod(inode_t *dir, const char *name, mode_t mode, int devid) {
    itype_t         type   = 0;

    // Validate the mode
    if ((mode & S_IFMT) == 0) {
        mode |= S_IFREG; // Default to regular file
    }
    
    if (!S_ISCHR(mode) && !S_ISBLK(mode) && !S_ISFIFO(mode) && !S_ISSOCK(mode) && !S_ISREG(mode)) {
        return -EINVAL; // Invalid argument
    }

    if (S_ISBLK(mode) || S_ISCHR(mode)) {
        if (S_ISBLK(mode))
            type = FS_BLK;
        else
            type = FS_CHR;
    } else if (S_ISREG(mode)) {
        type = FS_RGL;
    } else if (S_ISFIFO(mode)) {
        return -EOPNOTSUPP;
    }

    return tmpfs_create_node(dir, name, mode, type, devid);
}

int tmpfs_irename(inode_t *dir __unused, const char *old __unused, inode_t *newdir __unused, const char *new __unused) {
    return -ENOTSUP;
}
