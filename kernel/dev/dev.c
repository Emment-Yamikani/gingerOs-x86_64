#include <dev/dev.h>
#include <mm/kalloc.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <modules/module.h>
#include <bits/errno.h>

#define DEVMAX 256 

static dev_t *chrdev[DEVMAX];
static spinlock_t *chrdevlk = &SPINLOCK_INIT();

static dev_t *blkdev[DEVMAX];
static spinlock_t *blkdevlk = &SPINLOCK_INIT();

int dev_init(void) {
    int err = 0;

    printk("initaliazing devices...\n");

    memset(chrdev, 0, sizeof chrdev);
    memset(blkdev, 0, sizeof blkdev);

    if ((err = modules_init()))
        return err;

    return 0;
}

dev_t *kdev_get(struct devid *dd) {
    dev_t *dev = NULL;

    if (!dd) return NULL;

    switch (dd->type) {
    case FS_BLK:
        spin_lock(blkdevlk);
        dev = blkdev[dd->major];
        spin_unlock(blkdevlk);
        break;
    case FS_CHR:
        spin_lock(chrdevlk);
        dev = chrdev[dd->major];
        spin_unlock(chrdevlk);
        break;
    default:
        dev = NULL;
    }

    if (dev && !DEVID_CMP(&dev->devid, dd)) {
        forlinked(node, dev->devnext, node->devnext) {
            if (DEVID_CMP(&node->devid, dd)) {
                dev = node;
                break;
            }
        }
        dev = NULL;
    }

    return dev;
}

int    kdev_register(dev_t *dev, uint8_t major, uint8_t type) {
    int err = 0;

    if (!dev) return -EINVAL;

    switch (type) {
    case FS_BLK:
        spin_lock(blkdevlk);

        if (blkdev[major]) {
            err = -EALREADY;
            spin_unlock(blkdevlk);
            goto error;
        }

        if (dev->devprobe) {
            if ((err = dev->devprobe())) {
                spin_unlock(blkdevlk);
                goto error;
            }
        }

        blkdev[major] = dev;
        spin_unlock(blkdevlk);
        break;

    case FS_CHR:
        spin_lock(chrdevlk);

        if (chrdev[major]) {
            err = -EALREADY;
            spin_unlock(chrdevlk);
            goto error;
        }

        if (dev->devprobe) {
            if ((err = dev->devprobe())) {
                spin_unlock(chrdevlk);
                goto error;
            }
        }

        chrdev[major] = dev;
        spin_unlock(chrdevlk);
        break;
    default:
        printk("WARNING: Can't register file, not a device file\n");
        err = -ENXIO;
        goto error;
    }

    return 0;
error:
    return err;
}

int     kdev_close(struct devid *dd) {
    dev_t *dev = NULL;
    if (!(dev = kdev_get(dd)))
        return -ENXIO;
    
    if (!(dev->devops.close))
        return -ENOSYS;
    return dev->devops.close(dd);
}

int     kdev_open(struct devid *dd, int oflags, ...) {
    dev_t *dev = NULL;
    if (!(dev = kdev_get(dd)))
        return -ENXIO;
    
    if (!(dev->devops.open))
        return -ENOSYS;
    return dev->devops.open(dd, oflags);
}

off_t   kdev_lseek(struct devid *dd, off_t off, int whence) {
    dev_t *dev = NULL;
    if (!(dev = kdev_get(dd)))
        return -ENXIO;
    
    if (!(dev->devops.lseek))
        return -ENOSYS;
    return dev->devops.lseek(dd, off, whence);
}

int     kdev_ioctl(struct devid *dd, int request, void *argp) {
    dev_t *dev = NULL;
    if (!(dev = kdev_get(dd)))
        return -ENXIO;
    
    if (!(dev->devops.ioctl))
        return -ENOSYS;
    return dev->devops.ioctl(dd, request, argp);
}

ssize_t kdev_read(struct devid *dd, off_t off, void *buf, size_t nbyte) {
    dev_t *dev = NULL;
    if (!(dev = kdev_get(dd)))
        return -ENXIO;
    
    if (!(dev->devops.read))
        return -ENOSYS;
    return dev->devops.read(dd, off, buf, nbyte);
}

ssize_t kdev_write(struct devid *dd, off_t off, void *buf, size_t nbyte) {
    dev_t *dev = NULL;
    if (!(dev = kdev_get(dd)))
        return -ENXIO;
    
    if (!(dev->devops.write))
        return -ENOSYS;
    return dev->devops.write(dd, off, buf, nbyte);
}

int kdev_open_bdev(const char *bdev_name, struct devid *pdev) {
    dev_t *dev = NULL;

    if (bdev_name == NULL || pdev == NULL)
        return -EINVAL;
    
    spin_lock(blkdevlk);
    for (int i =0; i < DEVMAX; ++i) {
        dev = blkdev[i];
        if (dev == NULL)
            continue;
    
        if (!compare_strings(bdev_name, dev->devname)) {
            spin_unlock(blkdevlk);
            *pdev = dev->devid;
            return 0;
        }
    }
    spin_unlock(blkdevlk);
    return -ENOENT;
}

int kdev_getinfo(struct devid *dd, void *info) {
    dev_t *dev = NULL;

    if (dd == NULL || info == NULL)
        return -EINVAL;
    
    if ((dev = kdev_get(dd)) == NULL)
        return -ENOENT;

    return dev->devops.getinfo(dd, info);
}

int kdev_create(const char *dev_name,
                uint8_t type, uint8_t major,
                uint8_t minor, dev_t **pdev) {
    int err = 0;
    dev_t *dev = NULL;

    if (type != FS_BLK && type != FS_CHR)
        return -EINVAL;

    if ((dev = kdev_get(DEVID(type, DEV_T(major, minor)))))
        return -EEXIST;

    if (dev_name == NULL || pdev == NULL)
        return -EINVAL;

    if ((dev = kmalloc(sizeof *dev)) == NULL)
        return -ENOMEM;

    memset(dev, 0, sizeof *dev);

    dev->devlock = SPINLOCK_INIT();
    dev_lock(dev);

    dev->devid = *DEVID(type, DEV_T(major, minor));
    *pdev = dev;

    return 0;
    if (dev) kfree(dev);
    return err;
}