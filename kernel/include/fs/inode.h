#pragma once
#include <lib/stdint.h>
#include <lib/stdlib.h>
#include <lib/stddef.h>
#include <sync/spinlock.h>
#include <lib/types.h>
#include <ds/queue.h>
#include <fs/stat.h>
#include <fs/icache.h>
#include <sync/cond.h>
#include <fs/cred.h>

struct  iops;
struct  dentry;
struct  superblock;
typedef struct superblock superblock_t;
struct  filesystem;
typedef struct filesystem filesystem_t;
struct  dirent;

typedef enum {
    FS_INV,
    FS_RGL,
    FS_DIR,
    FS_CHR,
    FS_LNK,
    FS_BLK,
    FS_FIFO,
    FS_PIPE,
    FS_SOCK,
} itype_t;

#define iextract_type(mode) ({ \
    itype_t type = 0;          \
    if (S_ISDIR(mode))         \
        type = FS_DIR;         \
    else if (S_ISCHR(mode))    \
        type = FS_CHR;         \
    else if (S_ISBLK(mode))    \
        type = FS_BLK;         \
    else if (S_ISREG(mode))    \
        type = FS_RGL;         \
    else if (S_ISFIFO(mode))   \
        type = FS_FIFO;        \
    else if (S_ISLNK(mode))    \
        type = FS_LNK;         \
    else if (S_ISSOCK(mode))   \
        type = FS_SOCK;        \
    type;                      \
})

extern char * itype_strings [];

typedef struct inode {
    uintptr_t       i_ino;      // Filesystem specific i-number for this file.
    uid_t           i_uid;      // User identifier of owner.
    gid_t           i_gid;      // Group identifier of group that owns this file.
    itype_t         i_type;     // Type of file this inode represents.
    devid_t         i_rdev;     // Device description.
    mode_t          i_mode;     // Inode's access mode.
    size_t          i_size;     // Inode's data size.
    int             i_flags;    // Inode flags.
    ssize_t         i_refcnt;   // Number of references to this inode.
    ssize_t         i_hlinks;   // Number of hard links to this inode. 

    superblock_t    *i_sb;      // Superblock to while this inode belongs.
    struct iops     *i_ops;     // Filesystem specific inode operations. 
    void            *i_priv;    // Filesystem specific private data.
    struct dentry   *i_alias;   // Alias to this inode (can be multiple).
    icache_t        *i_cache;   // Page cache for this inode.

    cond_t          *i_writers; // writers condition variable.
    cond_t          *i_readers; // readers condition variable.

    spinlock_t      i_lock;     // Spinlock to protect access to inode's metadata.
    spinlock_t      i_datalock; // Spinlock to protect access to inode's data
    /**
     * TODO: May need to add another type of locking mechanism
     * specifically for data manipulation in this file.
     * (RW-locks, semaphores and mutex locking mechanisms).
     * 
     * And also a file lock(or maybe this will go in the struct file::file_lock)
     * for file-address space locking.
    */
} inode_t;

typedef struct iops {
    int     (*irmdir)(inode_t *ip);
    int     (*isync)(inode_t *ip);
    int     (*iclose)(inode_t *ip);
    int     (*iunlink)(inode_t *ip);
    int     (*itruncate)(inode_t *ip);
    int     (*iopen)(inode_t *ip, inode_t **pip);
    int     (*igetattr)(inode_t *ip, void *attr);
    int     (*isetattr)(inode_t *ip, void *attr);
    int     (*ifcntl)(inode_t *ip, int cmd, void *argp);
    int     (*iioctl)(inode_t *ip, int req, void *argp);
    ssize_t (*iread)(inode_t *ip, off_t off, void *buf, size_t nb);
    ssize_t (*iwrite)(inode_t *ip, off_t off, void *buf, size_t nb);
    int     (*imkdir)(inode_t *dir, const char *fname, mode_t mode);
    int     (*icreate)(inode_t *dir, const char *fname, mode_t mode);
    int     (*ibind)(inode_t *dir, struct dentry *dentry, inode_t *ip);
    int     (*ilookup)(inode_t *dir, const char *fname, inode_t **pipp);
    int     (*isymlink)(inode_t *ip, inode_t *atdir, const char *symname);
    int     (*ilink)(const char *oldname, inode_t *dir, const char *newname);
    int     (*imknod)(inode_t *dir, const char *name, mode_t mode, int devid);
    ssize_t (*ireaddir)(inode_t *dir, off_t off, struct dirent *buf, size_t count);
    int     (*irename)(inode_t *dir, const char *old, inode_t *newdir, const char *new);
    int     (*ichown)(inode_t *ip, uid_t owner, gid_t group);
} iops_t;

#define iassert(ip)         ({ assert((ip), "No inode"); })
#define ilock(ip)           ({ iassert(ip); spin_lock(&(ip)->i_lock); })
#define iunlock(ip)         ({ iassert(ip); spin_unlock(&(ip)->i_lock); })
#define iislocked(ip)       ({ iassert(ip); spin_islocked(&(ip)->i_lock); })
#define iassert_locked(ip)  ({ iassert(ip); spin_assert_locked(&(ip)->i_lock); })

#define icheck_op(ip, func) ({          \
    int err = 0;                        \
    iassert_locked(ip);                 \
    if ((ip) == NULL)                   \
        err = -EINVAL;                  \
    else if ((ip)->i_ops == NULL)       \
        err = -ENOSYS;                  \
    else if ((ip)->i_ops->func == NULL) \
        err = -ENOSYS;                  \
    err;                                \
})

#define IISTYPE(ip, type) ({ \
    iassert_locked(ip);      \
    (ip)->i_type == (type);  \
})

#define IISINV(ip) ({ IISTYPE(ip, FS_INV); })
#define IISREG(ip) ({ IISTYPE(ip, FS_RGL); })
#define IISDIR(ip) ({ IISTYPE(ip, FS_DIR); })
#define IISCHR(ip) ({ IISTYPE(ip, FS_CHR); })
#define IISSYM(ip) ({ IISTYPE(ip, FS_LNK); })
#define IISBLK(ip) ({ IISTYPE(ip, FS_BLK); })
#define IISFIFO(ip)({ IISTYPE(ip, FS_FIFO); })
#define IISPIPE(ip)({ IISTYPE(ip, FS_PIPE); })
#define IISSOCK(ip)({ IISTYPE(ip, FS_SOCK); })
#define IISDEV(ip) ({ IISCHR(ip) || IISBLK(ip); })

#define igetsize(ip) ({ \
    iassert_locked(ip); \
    ip->i_size;         \
})

#define iupdate_size(ip, size) ({ \
    iassert_locked(ip);           \
    ip->i_size = (size);          \
})


#define I_NOCACHE       BS(0)   // create an inode with no cache.
#define I_NORQUEUE      BS(1)   // create an inode with no read wait-queue.
#define I_NOWQUEUE      BS(2)   // create an inode with no write wait-queue.
#define I_NORWQUEUES    (I_NORQUEUE | I_NOWQUEUE) // create an inode with no rw wait queues.


int     ialloc(itype_t type, int flags, inode_t **pip);
void    iputcnt(inode_t *ip);
void    idupcnt(inode_t *ip);
void    iputlink(inode_t *ip);
void    iduplink(inode_t *ip);
void    irelease(inode_t *ip);
int     idel_alias(inode_t *ip, struct dentry *dentry);
int     iadd_alias(inode_t *ip, struct dentry *dentry);
int     imkalias(inode_t *ip, const char *name, struct dentry **dentry);

int     icheck_perm(inode_t *ip, cred_t *cred, int oflags);

int     iopen(inode_t *ip, inode_t **pip);
int     istat(inode_t *ip, struct stat *buf);
int     isync(inode_t *ip);
int     iclose(inode_t *ip);
int     iunlink(inode_t *ip);
int     itruncate(inode_t *ip);
int     igetattr(inode_t *ip, void *attr);
int     isetattr(inode_t *ip, void *attr);
int     ifcntl(inode_t *ip, int cmd, void *argp);
int     iioctl(inode_t *ip, int req, void *argp);
int     ilookup(inode_t *dir, const char *fname, inode_t **pipp);
ssize_t iread(inode_t *ip, off_t off, void *buf, size_t nb);
ssize_t iread_data(inode_t *ip, off_t off, void *buf, size_t nb);
ssize_t iwrite(inode_t *ip, off_t off, void *buf, size_t nb);
ssize_t iwrite_data(inode_t *ip, off_t off, void *buf, size_t nb);
int     ibind(inode_t *dir, struct dentry *dentry, inode_t *ip);
int     imkdir(inode_t *dir, const char *fname, mode_t mode);
int     icreate(inode_t *dir, const char *fname, mode_t mode);
ssize_t ireaddir(inode_t *dir, off_t off, void *buf, size_t count);
int     isymlink(inode_t *ip, inode_t *atdir, const char *symname);
int     ilink(const char *oldname, inode_t *dir, const char *newname);
int     imknod(inode_t *dir, const char *name, mode_t mode, int devid);
int     irename(inode_t *dir, const char *old, inode_t *newdir, const char *new);
int     ichown(inode_t *ip, uid_t owner, gid_t group);