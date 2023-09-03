#pragma once

#include <ds/list.h>
#include <lib/stdint.h>
#include <lib/stddef.h>
#include <lib/types.h>
#include <sync/spinlock.h>

typedef enum
{
    FS_INV = 0, // Invalid file type.
    FS_REG = 1, // Regular file.
    FS_DIR = 2, // Derectory.
    FS_CHR = 3, // Character device file.
    FS_BLK = 4, // Block device file.
    FS_FIF = 5, // FIFO.
    FS_SYM = 6, // Symbolic link.
} i_type_t;

typedef struct ialloc_desc
{
    uid_t p_uid;
    gid_t p_gid;
    mode_t p_mode;
    int   p_major;
    int   p_minor;
    uint8_t p_type;
    unsigned long p_flags;
} ialloc_desc_t;

#define IPROT(type, flags, uid, gid, major, minor, mode) ((ialloc_desc_t){ \
    .p_type = type,                                                        \
    .p_flags = flags,                                                      \
    .p_gid = gid,                                                          \
    .p_uid = uid,                                                          \
    .p_major = major,                                                      \
    .p_minor = minor,                                                      \
    .p_mode = mode,                                                        \
})

struct dentry;

struct inode;

typedef struct iops
{
    int     (*iclose)   (INODE inode);
    int     (*iperm)    (INODE inode, int mask);
    int     (*itrunc)   (INODE inode, off_t length);
    int     (*irmdir)   (INODE dir, struct dentry *dentry);
    int     (*iunlink)  (INODE dir, struct dentry *dentry);
    int     (*ilookup)  (INODE dir, struct dentry *dentry);
    int     (*iioctl)   (INODE ip, int request, void *argp);
    int     (*ibind)    (INODE dir, dentry_t *dentry, INODE ip);
    int     (*icreate)  (INODE dir, struct dentry *dentry, int mode);
    int     (*imkdir)   (INODE dir, struct dentry *dentry, mode_t mode);
    ssize_t (*iread)    (INODE ip, off_t off, void *buff, size_t nbytes);
    ssize_t (*iwrite)   (INODE ip, off_t off, void *buff, size_t nbytes);
    int     (*imknod)   (INODE dir, struct dentry *dentry, int mode, devid_t dev);
    int     (*isymlink) (INODE dir, struct dentry *dentry, const char *symname);
    int     (*ilink)    (struct dentry *old_dentry, INODE dir, struct dentry *new_dentry);
    int     (*irename)  (INODE old_dir, struct dentry *old_dentry, INODE new_dir, struct dentry *new_dentry);
} iops_t;

struct inode
{
    list_head_t i_dentry;
    list_head_t i_inodes;

    uid_t i_uid;
    gid_t i_gid;
    mode_t i_mode;

    uintptr_t i_ino;
    long i_rdev;
    uint8_t i_type;

    size_t i_size;

    long i_count;
    long i_nlink;

    unsigned i_flags;
    unsigned i_state;

    void *i_priv;

    struct iops *i_ops;

    spinlock_t   i_lock;
};

#define iassert(ip)         ({ assert(ip, "No inode"); })
#define ilock(ip)           ({ iassert(ip); spin_lock(&ip->i_lock); })
#define iunlock(ip)         ({ iassert(ip); spin_unlock(&ip->i_lock); })
#define iassert_locked(ip)  ({ iassert(ip); spin_assert_locked(&ip->i_lock); })

#define check_iop(ip, func) ({ \
    iassert_locked(ip);        \
    int ret = 0;               \
    if (!ip)                   \
        ret = -EINVAL;         \
    else if (ip->i_ops)        \
    {                          \
        if (!ip->i_ops->func)  \
            ret = -ENOSYS;     \
    }                          \
    else                       \
        ret = -ENOSYS;         \
    ret;                       \
})

#define INODE_ISTYPE(ip, __type) ({ iassert_locked(ip); (ip->i_type == __type); })
#define INODE_ISDIR(ip)     INODE_ISTYPE(ip, FS_DIR)
#define INODE_ISREG(ip)     INODE_ISTYPE(ip, FS_REG)
#define INODE_ISSYM(ip)     INODE_ISTYPE(ip, FS_SYM)
#define INODE_ISBLK(ip)     INODE_ISTYPE(ip, FS_BLK)
#define INODE_ISCHR(ip)     INODE_ISTYPE(ip, FS_CHR)
#define INODE_ISFIFO(ip)    INODE_ISTYPE(ip, FS_FIF)
#define INODE_ISDEV(ip)     (INODE_ISCHR(ip) || INODE_ISBLK(ip))

#define INODE_MAJOR(ip) ({iassert_locked(ip); (ip->i_rdev & 0xFF)})
#define INODE_MINOR(ip) ({iassert_locked(ip); ((ip->i_rdev >> 8) & 0xFF)})

#define INODE_PROT(ip) ({iassert(ip); IPROT(ip->i_type, ip->i_flags, ip->i_uid, ip->i_gid, INODE_MAJOR(ip), INODE_MINOR(ip), ip->i_mode); })

void idup(INODE);
int iclose(INODE);
void irelease(INODE);

/********************************************************************|
 *                          Inode helper function.
 * ******************************************************************|
 */

#define IEXEC 0x001   // inode was opened for execution.
#define IWRONLY 0x002 // inode was opened for writing.
#define IRDONLY 0x004 // inode was opened for reading.
#define IRDWR (IRDONLY | IWRONLY)
#define IRWX (IRDWR | IEXEC)
#define IRDX (IRDONLY | IEXEC)
#define IMAP 0x008 // inode has mapped pages.
#define IRA 0x010  // inode can read ahead.

#define ISTATE_BUSY 0x1
#define ISTATE_DIRTY 0x2

#define iget_uid(inode) ({iassert_locked(inode); inode->i_uid; })

#define iget_gid(inode) ({iassert_locked(inode); inode->i_gid; })

#define iget_mode(inode) ({iassert_locked(inode); inode->i_mode; })

#define iget_flags(inode) ({iassert_locked(inode); inode->i_flags; })

#define iget_state(inode) ({iassert_locked(inode); inode->i_state; })

#define itest_state(inode, state) ({iassert_locked(inode); (inode->i_state & (state)); })

#define itest_flags(inode, flags) ({iassert_locked(inode); (inode->i_flags & (flags)); })

#define iset_mode(inode, mask) ({ \
    int err = 0;                  \
    iassert_locked(inode);        \
    inode->i_mode &= (mask);      \
    err;                          \
})

#define iset_uid(inode, uid) ({ \
    int err = 0;                \
    iassert_locked(inode);      \
    inode->i_uid = (uid);       \
    err;                        \
})

#define iset_gid(inode, gid) ({ \
    int err = 0;                \
    iassert_locked(inode);      \
    inode->i_gid = (gid);       \
    err;                        \
})

#define iset_flags(inode, mask) ({ \
    int err = 0;                   \
    iassert_locked(inode);         \
    inode->i_flags &= (mask);      \
    err;                           \
})

#define iset_state(inode, mask) ({ \
    int err = 0;                   \
    iassert_locked(inode);         \
    inode->i_state &= (mask);      \
    err;                           \
})

/********************************************************************|
 *                          Generic syscalls.
 * ******************************************************************|
 */

int iperm(INODE inode, int mask);
int itrunc(INODE inode, off_t length);
int iopen(INODE, struct dentry *dentry);
int ialloc(ialloc_desc_t prot, INODE *pip);
int irmdir(INODE dir, struct dentry *dentry);
int iunlink(INODE dir, struct dentry *dentry);
int ilookup(INODE dir, struct dentry *dentry);
int icreate(INODE dir, struct dentry *dentry, int mode);
int iioctl(INODE ip, int request, void *argp);
int imkdir(INODE dir, struct dentry *dentry, mode_t mode);
ssize_t iread(INODE ip, off_t off, void *buff, size_t nbytes);
ssize_t iwrite(INODE ip, off_t off, void *buff, size_t nbytes);
int imknod(INODE dir, struct dentry *dentry, int mode, devid_t dev);
int isymlink(INODE dir, struct dentry *dentry, const char *symname);
int ibind(INODE dir, dentry_t *dentry, INODE ip);
int ilink(struct dentry *old_dentry, INODE dir, struct dentry *new_dentry);
int irename(INODE old_dir, struct dentry *old_dentry, INODE new_dir, struct dentry *new_dentry);