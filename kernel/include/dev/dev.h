#pragma once

#include <fs/fs.h>
#include <dev/rtc.h>
#include <lib/types.h>
#include <lib/stdint.h>
#include <lib/stddef.h>
#include <lib/printk.h>
#include <sync/spinlock.h>
#include <modules/module.h>

#define DEV_RANDOM  1
#define DEV_ZORE    1
#define DEV_MEM     1
#define DEV_NULL    1
#define DEV_TTY     4
#define DEV_CGA     5
#define DEV_CONSOLE 5
#define DEV_PTMX    5
#define DEV_HPET    10
#define DEV_PSAUX   10
#define DEV_MOUSE0  13
#define DEV_FB      29
#define DEV_PTS     136
#define DEV_CPU     202
#define DEV_RTC0    248

#define DEV_RAMDISK 0
#define DEV_SDA     8
#define DEV_CDROM   11

typedef struct {
    int     (*close)(struct devid *dd);
    int     (*getinfo)(struct devid *dd, void *info);
    int     (*open)(struct devid *dd, int oflags, ...);
    off_t   (*lseek)(struct devid *dd, off_t off, int whence);
    int     (*ioctl)(struct devid *dd, int request, void *arg);
    ssize_t (*read)(struct devid *dd, off_t off, void *buf, size_t nbyte);
    ssize_t (*write)(struct devid *dd, off_t off, void *buf, size_t nbyte);
} devops_t;

typedef struct dev {
    struct devid    devid;
    devops_t        devops;
    char            *devname;
    int             (*devprobe)();
    int             (*devmount)();
    struct dev      *devprev, *devnext;
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

#define DEVID_CMP(dd0, dd1) ({((dd0)->major == (dd1)->major && (dd0)->minor == (dd1)->minor);})

#define DEV_T(major, minor) ((devid_t)(((devid_t)(minor) << 8) & 0xff00) | ((devid_t)(major) & 0xff))

extern int dev_init(void);
extern dev_t *kdev_get(struct devid *dd);
extern int kdev_register(dev_t *, uint8_t major, uint8_t type);

extern int     kdev_close(struct devid *dd);
extern int     kdev_open(struct devid *dd, int oflags, ...);
extern off_t   kdev_lseek(struct devid *dd, off_t off, int whence);
extern int     kdev_ioctl(struct devid *dd, int request, void *argp);
extern ssize_t kdev_read(struct devid *dd, off_t off, void *buf, size_t nbyte);
extern ssize_t kdev_write(struct devid *dd, off_t off, void *buf, size_t nbyte);

typedef struct {
    size_t bi_size;
    size_t bi_blocksize;
} bdev_info_t;

int kdev_getinfo(struct devid *dd, void *info);
int kdev_open_bdev(const char *bdev_name, struct devid *pdd);
int kdev_create(const char *devname, uint8_t type, uint8_t major, uint8_t minor, dev_t **pdd);