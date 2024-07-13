#pragma once

#include <ds/queue.h>
#include <ds/ringbuf.h>
#include <fs/inode.h>
#include <lib/types.h>
#include <sync/spinlock.h>
#include <sys/system.h>

#define PIPE_R      BS(0) //
#define PIPE_W      BS(1) //
#define PIPE_RW     (PIPE_R | PIPE_W)

#define PIPESZ      (KiB(1))

typedef struct __pipe_t {
    flags32_t   p_flags;
    queue_t     p_readersq;
    queue_t     p_writersq;
    inode_t     *p_iread;
    inode_t     *p_iwrite;
    ringbuf_t   p_ringbuf;
    spinlock_t  p_lock;
} pipe_t;


#define pipe_assert(p)                  ({ assert(p, "No pipe descriptor."); })
#define pipe_lock(p)                    ({ pipe_assert(p); spin_lock(&(p)->p_lock); })
#define pipe_unlock(p)                  ({ pipe_assert(p); spin_unlock(&(p)->p_lock); })
#define pipe_islocked(p)                ({ pipe_assert(p); spin_islocked(&(p)->p_lock); })
#define pipe_assert_locked(p)           ({ pipe_assert(p); spin_assert_locked(&(p)->p_lock); })

#define pipe_setflags(p, flags)         ({ pipe_assert_locked(p); (p)->p_flags |= (flags); })
#define pipe_testflags(p, flags)        ({ pipe_assert_locked(p); ((p)->p_flags & (flags)); })
#define pipe_maskflags(p, flags)        ({ pipe_assert_locked(p); (p)->p_flags &= ~(flags); })

#define pipe_iread(p)                   ({ pipe_assert_locked(p); ((p)->p_iread); })
#define pipe_iwrite(p)                  ({ pipe_assert_locked(p); ((p)->p_iwrite); })

#define pipe_readersq(p)                ({ pipe_assert_locked(p); (&(p)->p_readersq); })
#define pipe_writersq(p)                ({ pipe_assert_locked(p); (&(p)->p_writersq); })

#define pipe_lock_readersq(p)           ({ queue_lock(pipe_readersq(p)); })
#define pipe_unlock_readersq(p)         ({ queue_unlock(pipe_readersq(p)); })
#define pipe_assert_locked_readersq(p)  ({ queue_assert_locked(pipe_readersq(p)); })
#define pipe_lock_writersq(p)           ({ queue_lock(pipe_writersq(p)); })
#define pipe_unlock_writersq(p)         ({ queue_unlock(pipe_writersq(p)); })
#define pipe_assert_locked_writersq(p)  ({ queue_assert_locked(pipe_writersq(p)); })


#define pipe_iswritable(p)              ({ pipe_assert_locked(p); pipe_testflags(p, PIPE_W); })
#define pipe_isreadable(p)              ({ pipe_assert_locked(p); pipe_testflags(p, PIPE_R); })

int pipefs_init(void);

int     pipefs_isync(inode_t *ip);
int     pipefs_iclose(inode_t *ip);
int     pipefs_iunlink(inode_t *ip);
int     pipefs_itruncate(inode_t *ip);
int     pipefs_igetattr(inode_t *ip, void *attr);
int     pipefs_isetattr(inode_t *ip, void *attr);
int     pipefs_ifcntl(inode_t *ip, int cmd, void *argp);
int     pipefs_iioctl(inode_t *ip, int req, void *argp);
ssize_t pipefs_iread(inode_t *ip, off_t off, void *buf, size_t nb);
ssize_t pipefs_iwrite(inode_t *ip, off_t off, void *buf, size_t nb);
int     pipefs_imkdir(inode_t *dir, const char *fname, mode_t mode);
int     pipefs_icreate(inode_t *dir, const char *fname, mode_t mode);
int     pipefs_ibind(inode_t *dir, struct dentry *dentry, inode_t *ip);
int     pipefs_ilookup(inode_t *dir, const char *fname, inode_t **pipp);
int     pipefs_isymlink(inode_t *ip, inode_t *atdir, const char *symname);
int     pipefs_imknod(inode_t *dir, const char *name, mode_t mode, int devid);
ssize_t pipefs_ireaddir(inode_t *dir, off_t off, struct dirent *buf, size_t count);
int     pipefs_ilink(const char *oldname, inode_t *dir, const char *newname);
int     pipefs_irename(inode_t *dir, const char *old, inode_t *newdir, const char *new);