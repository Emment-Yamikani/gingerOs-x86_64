#pragma once

#include <fs/dentry.h>
#include <fs/inode.h>
#include <fs/mount.h>
#include <sync/spinlock.h>
#include <ds/queue.h>
#include <lib/types.h>

#define MAXFNAME 255

struct dirent {

};

typedef struct uio {
    char    *u_cwd, *u_root;
    uid_t   u_uid, u_euid, u_suid;
    gid_t   u_gid, u_egid, u_sgid;
    mode_t  u_umask;
} uio_t;

#define UIO_DEFAULT() ((uio_t){ \
    .u_cwd = "/",               \
    .u_root = "/",              \
    .u_uid = 0,                 \
    .u_euid = 0,                \
    .u_egid = 0,                \
    .u_gid = 0,                 \
    .u_suid = 0,                \
    .u_sgid = 0,                \
})

struct filesystem;
struct devid;
struct superblock;

typedef struct {
    dentry_t *(*mount)(struct superblock *);
    inode_t *(*inode_alloc)(struct superblock *);
    int (*unmount)(struct superblock *);
    int (*getattr)(struct superblock *);
    int (*setattr)(struct superblock *);
} sb_ops_t;

typedef struct superblock {
    long                sb_id;
    uio_t               sb_uio;
    sb_ops_t            sb_ops;
    iops_t              *sb_iops;
    long                sb_count;
    uintptr_t           sb_magic;
    uintptr_t           sb_flags;
    uintptr_t           sb_iflags;
    void                *sb_priv;
    struct devid        sb_devid;
    dentry_t            *sb_root;
    size_t              sb_size;
    char                sb_magic0[31];
    size_t              sb_blocksize;
    fs_mount_t          *sb_mnt;
    struct filesystem   *sb_filesystem;
    spinlock_t          sb_lock;
} superblock_t;

#define sbassert(sb)        ({ assert((sb), "No superblock"); })
#define sblock(sb)          ({ sbassert(sb); spin_lock(&(sb)->sb_lock); });
#define sbunlock(sb)        ({ sbassert(sb); spin_unlock(&(sb)->sb_lock); })
#define sblocked(sb)        ({ sbassert(sb); spin_locked(&(sb)->sb_lock); })
#define sbassert_locked(sb) ({ sbassert(sb); spin_assert_locked(&(sb)->sb_lock); })


typedef struct filesystem {
    long        fs_id;
    long        fs_flags;
    long        fs_count;
    char        *fs_name;
    iops_t      *fs_iops;
    void        *fs_priv;
    queue_t     *fs_superblocks;
    int         (*get_sb)(struct filesystem *fs, const char *src, unsigned long flags, void *data, superblock_t **psbp);
    int         (*mount)(struct filesystem *fs, dentry_t *src, dentry_t *dst, unsigned long flags, void *data);
    spinlock_t  fs_lock;
} filesystem_t;

#define fsassert(fs)        ({ assert((fs), "No filesystem"); })
#define fslock(fs)          ({ fsassert(fs); spin_lock(&(fs)->fs_lock); });
#define fsunlock(fs)        ({ fsassert(fs); spin_unlock(&(fs)->fs_lock); })
#define fsislocked(fs)      ({ fsassert(fs); spin_islocked(&(fs)->fs_lock); })
#define fsassert_locked(fs) ({ fsassert(fs); spin_assert_locked(&(fs)->fs_lock); })

void fs_dup(filesystem_t *fs);
void fs_put(filesystem_t *fs);
void fs_free(filesystem_t *fs);
long fs_count(filesystem_t *fs);
void fs_unsetname(filesystem_t *fs);
int fs_set_iops(filesystem_t *fs, iops_t *iops);
int fs_setname(filesystem_t *fs, const char *fsname);
int fs_create(const char *name, iops_t *iops, filesystem_t **pfs);
int fs_add_superblock(filesystem_t *fs, superblock_t *sb);
int fs_del_superblock(filesystem_t *fs, superblock_t *sb);

int verify_path(const char *path);
int parse_path(const char *path, const char *cwd,
               char **abspath, char ***abspath_tokens,
               char **last_token, int *isdir);

int vfs_init(void);
int ramfs_init(void);
int vfs_alloc_vnode(const char *name, itype_t type, inode_t **pip, dentry_t **pdp);
dentry_t *vfs_getdroot(void);
int vfs_mount_droot(dentry_t *dentry);
int vfs_register_fs(filesystem_t *fs);
int vfs_unregister_fs(filesystem_t *fs);
int  vfs_getfs(const char *type, filesystem_t **pfs);
int vfs_lookup(const char *fn, uio_t *uio,
               int oflags, mode_t mode,
               int flags, inode_t **pip,
               dentry_t **pdp);

/**
 * VFS mount helpers
*/

fs_mount_t *alloc_fsmount(void);

/**
 * Super block helpers
*/

int getsb_bdev(filesystem_t *fs, const char *bdev_name, unsigned long flags, void *data, superblock_t **psbp, int (*sb_fill)(filesystem_t *fs, struct devid *dd, superblock_t *sb));
int getsb_nodev(filesystem_t *fs, unsigned long flags, void *data, superblock_t **psbp, int (*sb_fill)(filesystem_t *fs, struct devid *dd, superblock_t *sb));
int getsb_pseudo(filesystem_t *fs, unsigned long flags, void *data, superblock_t **psbp, int (*sb_fill)(filesystem_t *fs, struct devid *dd, superblock_t *sb));
int getsb_single(filesystem_t *fs, unsigned long flags, void *data, superblock_t **psbp, int (*sb_fill)(filesystem_t *fs, struct devid *dd, superblock_t *sb));