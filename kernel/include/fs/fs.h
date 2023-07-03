#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>
#include "dentry.h"
#include "inode.h"
#include "path.h"
#include "tmpfs.h"
#include "devfs.h"
#include <ds/list.h>
#include <sys/_stat.h>
#include <sys/_fcntl.h>

#define MAXFNAME 1024

typedef struct __uio
{
    uid_t u_uid;
    uid_t u_euid;
    gid_t u_gid;
    gid_t u_egid;
    char *u_cwd;
} uio_t, *UIO;

#define __UIO(__wd) (&(uio_t){ \
    .u_cwd = __wd,             \
    .u_egid = 0,               \
    .u_euid = 0,               \
    .u_gid = 0,                \
    .u_uid = 0,                \
})

#define ROOT_UIO() __UIO("/")

struct filesystem;

typedef struct superblock
{
    unsigned long       sb_id;
    uid_t               sb_uid;
    gid_t               sb_gid;
    mode_t              sb_mode;
    list_head_t         sb_list;
    long                sb_count;
    long                sb_magic;
    size_t              sb_blocksz;
    size_t              sb_maxfilesz;
    unsigned long       sb_flags;

    char                *sb_uuid;
    void                *sb_priv;
    dentry_t            *sb_mpoint;
    struct filesystem   *sb_fsys;

    spinlock_t          sb_lock;
} superblock_t;

#define sb_assert(__sb)         ({ assert(__sb, "No superblock"); })
#define sb_lock(__sb)           ({sb_assert(__sb); spin_lock(&__sb->sb_lock); })
#define sb_unlock(__sb)         ({sb_assert(__sb); spin_unlock(&__sb->sb_lock); })
#define sb_assert_locked(__sb)  ({sb_assert(sb); spin_assert_locked(&__sb->sb_lock); })

typedef struct filesystem
{
    unsigned long   fs_id;      // per-fs Identity
    list_head_t     fs_list;    // list of all filesystems in vfs
    long            fs_count;   // total references to this structure
    unsigned        fs_flags;   // per filesystem flags
    iops_t          *fs_iops;   // inode operations owned by this fs
    char            *fs_type;   // name of filesystem
    char            *fs_uuid;   // unique user ID
    void            *fs_priv;   // private data
    list_head_t     fs_sblocks; // list of all superblocks of this filesystem type

    spinlock_t      fs_lock;
} filesystem_t;

#define fs_assert(__fs)         ({ assert(__fs, "No filesystem"); })
#define fs_lock(__fs)           ({fs_assert(__fs); spin_lock(&__fs->fs_lock); })
#define fs_unlock(__fs)         ({fs_assert(__fs); spin_unlock(&__fs->fs_lock); })
#define fs_assert_locked(__fs)  ({fs_assert(__fs); spin_assert_locked(&__fs->fs_lock); })

extern int vfs_init(void);
extern void vfs_filesystem_put(filesystem_t *fs);
extern int vfs_filesystem_unregister(filesystem_t *fs);
extern int vfs_filesystem_get(const char *type, filesystem_t **pfs);
extern int vfs_filesystem_register(const char *type, uio_t uio, int flags, iops_t *iops, filesystem_t **pfs);

extern int vfs_set_droot(dentry_t *dentry);
extern int check_iperm(inode_t *ip, uio_t *uio, int oflags);
extern int vfs_lookup(const char *path, UIO uio, int oflags, int flags, mode_t mode, INODE *pinode, dentry_t **pdentry);

extern ssize_t vfs_iread(INODE, void *, size_t);
extern int vfs_open(const char *path, int oflags, int mode, INODE *ref);
