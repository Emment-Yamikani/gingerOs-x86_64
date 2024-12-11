#pragma once

#include <fs/fs.h>
#include <dev/rtc.h>
#include <lib/types.h>
#include <lib/stdint.h>
#include <lib/stddef.h>
#include <lib/printk.h>
#include <sync/spinlock.h>
#include <modules/module.h>

/*Character Devices */

#define DEV_MEM     1   // minor=1
#define DEV_NULL    1   // minor=3
#define DEV_ZERO    1   // minor=5
#define DEV_FULL    1   // minor=7
#define DEV_RANDOM  1   // minor=8
#define DEV_TTY     4   // minor=[0..n-1]
#define DEV_CONSOLE 5   // minor=1
#define DEV_PTMX    5   // minor=2
#define DEV_CGA     5   // minor=3
#define DEV_UART    6   // minor=0
#define DEV_PSAUX   10  // minor=1
#define DEV_HPET    10  // minor=228
#define DEV_KBD0    13  // minor=0
#define DEV_MOUSE0  13  // minor=1
#define DEV_FB      29  // minor=0
#define DEV_PTS     136 // minor=[0..n-1]
#define DEV_CPU     202 // minor=[0..n-1]
#define DEV_CPU_MSR 203 // minor=[0..n-1]
#define DEV_RTC0    248 // minor=0

/*Block Devices*/

#define DEV_RAMDISK 1   // minor=[0..n-1]
#define DEV_SDA     8   // minor=[0..n-1]
#define DEV_CDROM   11  // minor=[0..n-1]

typedef struct {
    int     (*close)(struct devid *dd);
    int     (*open)(struct devid *dd, inode_t **pip);
    int     (*mmap)(struct devid *dd, vmr_t *region);
    int     (*getinfo)(struct devid *dd, void *info);
    off_t   (*lseek)(struct devid *dd, off_t off, int whence);
    int     (*ioctl)(struct devid *dd, int request, void *arg);
    ssize_t (*read)(struct devid *dd, off_t off, void *buf, size_t nbyte);
    ssize_t (*write)(struct devid *dd, off_t off, void *buf, size_t nbyte);
} devops_t;

/**
 * @brief Macro to declare function ops for a device driver.
 * This is for easy and faster implemention to avoid duplication
 * as much as possible.*/
#define DEV_DECL_OPS(__privacy__, __prefix__)                                                     \
    __privacy__ int __prefix__##_probe(void);                                                     \
    __privacy__ int __prefix__##_close(struct devid *dd);                                         \
    __privacy__ int __prefix__##_getinfo(struct devid *dd, void *info);                           \
    __privacy__ int __prefix__##_open(struct devid *dd, inode_t **pip);                           \
    __privacy__ off_t __prefix__##_lseek(struct devid *dd, off_t off, int whence);                \
    __privacy__ int __prefix__##_ioctl(struct devid *dd, int request, void *arg);                 \
    __privacy__ ssize_t __prefix__##_read(struct devid *dd, off_t off, void *buf, size_t nbyte);  \
    __privacy__ ssize_t __prefix__##_write(struct devid *dd, off_t off, void *buf, size_t nbyte); \
    __privacy__ int __prefix__##_mmap(struct devid *dd, vmr_t *region)

#define DEVOPS(__prefix__)(devops_t){\
    .close = __prefix__##_close,     \
    .open = __prefix__##_open,       \
    .mmap = __prefix__##_mmap,       \
    .getinfo = __prefix__##_getinfo, \
    .lseek = __prefix__##_lseek,     \
    .ioctl = __prefix__##_ioctl,     \
    .read = __prefix__##_read,       \
    .write = __prefix__##_write,     \
}

typedef struct dev {
    struct devid    dev_id;             // device id.
    devops_t        dev_ops;            // device operations.
    char            *dev_name;          // name of device.
    void            *dev_priv;          // device private data.
    int             (*dev_probe)();     // dev prober, call by kdev_register().
    int             (*dev_mount)();     // dev mount function. (TODO: may we don't need this?)
    void            (*dev_fini)(struct devid *dd);
    struct dev      *dev_prev;          // prev in this dev major(driver)'s list.
    struct dev      *dev_next;          // next in this dev major(driver)'s list.
    spinlock_t      dev_lck;            // dev struct lock.
} dev_t;

#define dev_assert(dev)         ({ assert((dev), "No dev"); })
#define dev_lock(dev)           ({ dev_assert(dev); spin_lock(&(dev)->dev_lck); })
#define dev_unlock(dev)         ({ dev_assert(dev); spin_unlock(&(dev)->dev_lck); })
#define dev_islocked(dev)       ({ dev_assert(dev); spin_islocked(&(dev)->dev_lck); })
#define dev_assert_locked(dev)  ({ dev_assert(dev); spin_assert_locked(&(dev)->dev_lck); })

#define DEVID(_type, _rdev) ((struct devid){   \
    .major = ((devid_t)(_rdev)) & 0xffu,       \
    .minor = (((devid_t)(_rdev)) >> 8) & 0xffu,\
    .type = ((itype_t)_type),                  \
})

#define DEVID_PTR(_type, _rdev) (&DEVID(_type, _rdev))

#define IDEVID(__ip__) (&(struct devid){                  \
    .inode = (inode_t *)(__ip__),                         \
    .major = ((devid_t)((__ip__)->i_rdev)) & 0xffu,       \
    .minor = (((devid_t)((__ip__)->i_rdev)) >> 8) & 0xffu,\
    .type = ((itype_t)(__ip__)->i_type),                  \
})

#define DEVID_CMP(dd0, dd1) ({((dd0)->major == (dd1)->major) && ((dd0)->minor == (dd1)->minor); })

#define DEV_T(major, minor) ((devid_t)(((devid_t)(minor) << 8) & 0xff00u) | ((devid_t)(major) & 0xffu))

#define DEV_INIT(name, _type, _major, _minor) \
dev_t name##dev = {                       \
    .dev_lck = SPINLOCK_INIT(),           \
    .dev_name = #name,                    \
    .dev_next = NULL,                     \
    .dev_prev = NULL,                     \
    .dev_probe = name##_probe,            \
    .dev_id = {                           \
        .type = (_type),                  \
        .major = (_major),                \
        .minor = (_minor),                \
    },                                    \
    .dev_ops = {                          \
        .close = name##_close,            \
        .getinfo = name##_getinfo,        \
        .open = name##_open,              \
        .lseek = name##_lseek,            \
        .ioctl = name##_ioctl,            \
        .read = name##_read,              \
        .write = name##_write,            \
        .mmap = name##_mmap,              \
    },                                    \
}

extern int dev_init(void);
extern int kdev_unregister(struct devid *dd);
extern dev_t *kdev_get(struct devid *dd);
extern int kdev_register(dev_t *, uint8_t major, uint8_t type);

extern int     kdev_close(struct devid *dd);
extern int     kdev_open(struct devid *dd, inode_t **pip);
extern int     kdev_mmap(struct devid *dd, vmr_t *region);
extern off_t   kdev_lseek(struct devid *dd, off_t off, int whence);
extern int     kdev_ioctl(struct devid *dd, int request, void *argp);
extern ssize_t kdev_read(struct devid *dd, off_t off, void *buf, size_t nbyte);
extern ssize_t kdev_write(struct devid *dd, off_t off, void *buf, size_t nbyte);

typedef struct {
    size_t bi_size;
    size_t bi_blocksize;
} bdev_info_t;

extern void kdev_free(dev_t *dev);
extern int kdev_getinfo(struct devid *dd, void *info);
extern int kdev_open_bdev(const char *bdev_name, struct devid *pdd);
extern int kdev_create(const char *devname, uint8_t type, uint8_t major, uint8_t minor, dev_t **pdd);