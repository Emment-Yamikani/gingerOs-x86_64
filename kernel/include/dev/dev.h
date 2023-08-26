#pragma once

#include <lib/printk.h>
#include <lib/stdint.h>
#include <lib/stddef.h>
#include <lib/types.h>
#include <fs/fs.h>
#include <sync/spinlock.h>
#include <modules/module.h>

#define DEV_RAMDISK 0

struct devid{
    uint8_t major;
    uint8_t minor;
    uint8_t type;
};

typedef struct {
    int     (*close)(struct devid *dd);
    int     (*open)(struct devid *dd, int oflags, ...);
    off_t   (*lseek)(struct devid *dd, off_t off, int whence);
    int     (*ioctl)(struct devid *dd, int request, void *arg);
    ssize_t (*read)(struct devid *dd, off_t off, void *buf, size_t nbyte);
    ssize_t (*write)(struct devid *dd, off_t off, void *buf, size_t nbyte);
} devops_t;

typedef struct {
    struct devid    devid;
    devops_t        devops;
    char            *devname;
    int             (*devprobe)();
    int             (*devmount)();
    spinlock_t      devlock;
} dev_t;

#define dev_assert(dev)         ({ assert((dev), "No dev"); })
#define dev_lock(dev)           ({ dev_assert(dev); spin_lock(&(dev)->devlock); })
#define dev_unlock(dev)         ({ dev_assert(dev); spin_unlock(&(dev)->devlock); })
#define dev_assert_locked(dev)  ({ dev_assert(dev); spin_assert_locked(&(dev)->devlock); })

#define DEVID(_type, rdev) (&(struct devid){ \
    .type = (_type),                         \
    .major = ((devid_t)(rdev)&0xff),        \
    .minor = ((devid_t)(rdev) >> 8) & 0xff, \
})

#define IDEVID(inode) ({    \
    DEVID((inode)->i_type,  \
          (inode)->i_rdev); \
})

#define DEV_T(minor, major) ((devid_t)(((devid_t)(minor) << 8) & 0xff00) | ((devid_t)(major) & 0xff))

extern int dev_init(void);
extern dev_t *kdev_get(struct devid *dd);
extern int kdev_register(dev_t *, uint8_t major, uint8_t type);

extern int     kdev_close(struct devid *dd);
extern int     kdev_open(struct devid *dd, int oflags, ...);
extern off_t   kdev_lseek(struct devid *dd, off_t off, int whence);
extern int     kdev_ioctl(struct devid *dd, int request, void *argp);
extern ssize_t kdev_read(struct devid *dd, off_t off, void *buf, size_t nbyte);
extern ssize_t kdev_write(struct devid *dd, off_t off, void *buf, size_t nbyte);