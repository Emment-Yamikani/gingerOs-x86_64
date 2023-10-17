#include <fs/fs.h>
#include <bits/errno.h>
#include <ds/btree.h>
#include <mm/kalloc.h>
#include <fs/tmpfs.h>

typedef struct tmpfs_inode_t {
    uio_t       uio;
    uintptr_t   ino;
    itype_t     type;
    size_t      size;
    size_t      nlink;
    void        *data;
} tmpfs_inode_t;

typedef struct tmpfs_dirent_t {
    char                    *name;
    struct tmpfs_dirent_t   *prev;
    struct tmpfs_dirent_t   *next;
    inode_t                 *inode;
} tmpfs_dirent_t;

static filesystem_t *tmpfs = NULL;

int tmpfs_init(void);
static int tmpfs_ialloc(itype_t type, inode_t **pip);

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

static int tmpfs_fill_sb(filesystem_t *fs __unused, const char *target,
    struct devid *devid __unused, superblock_t *sb) {
    int err = 0;
    inode_t *iroot = NULL;
    dentry_t *droot = NULL;

    if ((err = tmpfs_ialloc(FS_DIR, &iroot)))
        return err;
    
    if ((err = dalloc(target, &droot))) {
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

static int tmpfs_getsb(filesystem_t *fs, const char *src __unused,
                       const char *target, unsigned long flags,
                       void *data, superblock_t **psbp) {
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

static int tmpfs_ialloc(itype_t type, inode_t **pip) {
    int err =  0;
    char *name = NULL;
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

    inode->i_type   = type;
    inode->i_priv   = tmpfs_ip;
    inode->i_ops    = &tmpfs_iops;
    inode->i_ino    = atomic_fetch_inc(&tmpfs_inos);

    tmpfs_ip->nlink = 1;
    tmpfs_ip->data  = bt;
    tmpfs_ip->type  = type;
    tmpfs_ip->ino   = inode->i_ino;

    *pip = inode;
    return 0;
error:
    if (name)
        kfree(name);

    if (tmpfs_ip)
        kfree(tmpfs_ip);
    
    if (bt)
        btree_free(bt);
    return err;
}

static int tmpfs_dirent_alloc(const char *fname, inode_t *inode, tmpfs_dirent_t **ptde) {
    int err = 0;
    char *name = NULL;
    tmpfs_dirent_t *tde = NULL;

    iassert_locked(inode);

    if (fname == NULL || ptde == NULL)
        return -EINVAL;

    if ((name = strdup(fname)) == NULL)
        return -ENOMEM;

    err = -ENOMEM;
    if ((tde = kmalloc(sizeof *tde)) == NULL)
        goto error;

    if ((err = iopen(inode)))
        goto error;

    tde->prev = NULL;
    tde->next = NULL;
    tde->name = name;
    tde->inode = inode;

    *ptde = tde;
    return 0;
error:
    if (name)
        kfree(name);
    
    if (tde)
        kfree(tde);
    
    return err;
}

int tmpfs_delete(inode_t *dir, const char *fname);

static int tmpfs_create_node(inode_t *dir, struct dentry *dentry, mode_t mode, itype_t type) {
    int err = 0;
    size_t hash = 0;
    inode_t *ip = NULL;
    btree_t *bt = NULL;
    tmpfs_inode_t *tino = NULL;
    tmpfs_inode_t *tdir = NULL;
    tmpfs_dirent_t *last = NULL;
    tmpfs_dirent_t *tde = NULL, *dirent = NULL, *next = NULL;

    if (dir == NULL || dentry == NULL)
        return -EINVAL;

    iassert_locked(dir);
    dassert_locked(dentry);

    if (IISDIR(dir) == 0)
        return -ENOTDIR;
    
    if ((tdir = dir->i_priv) == NULL)
        return -EINVAL;

    if ((bt = tdir->data) == NULL)
        return -EINVAL;

    if ((err = tmpfs_ialloc(type, &ip)))
        goto error;

    if ((err = tmpfs_dirent_alloc(dentry->d_name, ip, &tde)))
        goto error;

    hash = tmpfs_hash(dentry->d_name);

    btree_lock(bt);
    if ((err = btree_search(bt, hash, (void **)&dirent)) == 0) {
        /// Validate we don't already have an entry of the same name.
        /// and return the last dirent.
        forlinked(node, dirent, next) {
            next = (last = node)->next;
            if (!compare_strings(dentry->d_name, node->name)) {
                err = -EEXIST;
                btree_unlock(bt);
                goto error;
            }
        }
        
        /**
         * There was a collision in the binary search tree at this hash
         * so handle it by using a linked list.
         * Unreal but yeah, behold a simple HASH algorithm that works(Ain't i a genius?).
        */
        last->next = tde;
        tde->prev = last;
        btree_unlock(bt);
        goto done;
    } else if (err == -ENOENT) {
        /**
         * If no existing entry was found
         * Just add the New dirent to this btree.
        */
        if ((err = btree_insert(bt, hash, tde))) {
            btree_unlock(bt);
            goto error;
        }
    } else {
        /**
         * otherwise another type of error occured
         * SO, bailout.
        */
        btree_unlock(bt);
        goto error;
    }
    btree_unlock(bt);

done:
    ip->i_mode = mode;
    tino = ip->i_priv;
    ip->i_hlinks = tino->nlink;

    return 0;
error:
    if (tino)
        kfree(tino);
    return err;
}

int tmpfs_imkdir(inode_t *dir, struct dentry *dentry, mode_t mode) {
    return tmpfs_create_node(dir, dentry, mode, FS_DIR);
}

int tmpfs_icreate(inode_t *dir, struct dentry *dentry, mode_t mode) {
    return tmpfs_create_node(dir, dentry, mode, FS_RGL);
}

int tmpfs_ilookup(inode_t *dir, struct dentry *dentry) {
    // int err = 0;
    // btree_t *bt = NULL;
    tmpfs_inode_t *tino = NULL;
    // tmpfs_dirent_t *tde = NULL;

    iassert_locked(dir);
    dassert_locked(dentry);

    if (dir == NULL || dentry == NULL)
        return -EINVAL;
    
    if (IISDIR(dir) == 0)
        return -ENOTDIR;

    if ((tino = dir->i_priv) == NULL)
        return -EINVAL;

    return 0;
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