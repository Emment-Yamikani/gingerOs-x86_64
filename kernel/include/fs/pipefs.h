#pragma once

#include <ds/ringbuf.h>
#include <ds/queue.h>
#include <fs/file.h>
#include <fs/inode.h>
#include <lib/types.h>
#include <sync/spinlock.h>
#include <sys/system.h>
#include <sync/cond.h>

#define PIPE_R      BS(0) //
#define PIPE_W      BS(1) //
#define PIPE_RW     (PIPE_R | PIPE_W)

#define PIPESZ      (KiB(4))

typedef struct __pipe_t {
    flags32_t   p_flags;
    cond_t      p_readers;
    cond_t      p_writers;
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

#define pipe_setflags(p, flags)         ({ pipe_assert_locked(p); (p)->p_flags  |= (flags); })
#define pipe_testflags(p, flags)        ({ pipe_assert_locked(p); ((p)->p_flags &  (flags)); })
#define pipe_maskflags(p, flags)        ({ pipe_assert_locked(p); (p)->p_flags  &= ~(flags); })

#define pipe_iread(p)                   ({ pipe_assert_locked(p); ((p)->p_iread); })
#define pipe_iwrite(p)                  ({ pipe_assert_locked(p); ((p)->p_iwrite); })

#define pipe_wake_reader(p)             ({ pipe_assert_locked(p); cond_signal(&(p)->p_readers); })
#define pipe_wake_all_readers(p)        ({ pipe_assert_locked(p); cond_broadcast(&(p)->p_readers); })
#define pipe_wake_writer(p)             ({ pipe_assert_locked(p); cond_signal(&(p)->p_writers); })
#define pipe_wake_all_writers(p)        ({ pipe_assert_locked(p); cond_broadcast(&(p)->p_writers); })
#define pipe_reader_wait(p)             ({ pipe_assert_locked(p); cond_wait_releasing(&(p)->p_readers, &(p)->p_lock); })
#define pipe_writer_wait(p)             ({ pipe_assert_locked(p); cond_wait_releasing(&(p)->p_writers, &(p)->p_lock); })

#define pipe_iswritable(p)              ({ pipe_assert_locked(p); pipe_testflags(p, PIPE_W); })
#define pipe_isreadable(p)              ({ pipe_assert_locked(p); pipe_testflags(p, PIPE_R); })

#define pipe_getbuff(p)                 ({ pipe_assert_locked(p); &(p)->p_ringbuf; })
#define pipe_lockbuf(p)                 ({ ringbuf_lock(pipe_getbuff(p)); })
#define pipe_unlockbuf(p)               ({ ringbuf_unlock(pipe_getbuff(p)); })

int     pipefs_init(void);
int     pipe_mkpipe(pipe_t **pref);

int     pipefs_iopen(inode_t *idev, int oflags, ...);
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