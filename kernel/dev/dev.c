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

    if (!dd) return NULL;

    // printk("%s:%d: %d->rdev[%d:%d]: %p\n",
        //    __FILE__, __LINE__, dd->type, dd->major, dd->minor, dd);

    switch (dd->type) {
    case FS_BLK:
        spin_lock(blkdevlk);
        forlinked(dev, blkdev[dd->major], dev->dev_next) {
            if (DEVID_CMP(&dev->dev_id, dd)) {
                spin_unlock(blkdevlk);
                return dev;
            }
        }
        spin_unlock(blkdevlk);
        break;
    case FS_CHR:
        spin_lock(chrdevlk);
        forlinked(dev, chrdev[dd->major], dev->dev_next) {
            if (DEVID_CMP(&dev->dev_id, dd)) {
                spin_unlock(chrdevlk);
                return dev;
            }
        }
        spin_unlock(chrdevlk);
        break;
    }

    return NULL;
}

int    kdev_register(dev_t *dev, u8 major, u8 type) {
    int     err     = 0;
    dev_t   *tail   = NULL;
    dev_t   **table = NULL;
    spinlock_t *lock= NULL;

    if (!dev || (dev->dev_id.type != type)) return -EINVAL;

    switch (type) {
    case FS_BLK:
        table   = blkdev;
        lock    = blkdevlk;
        break;

    case FS_CHR:
        table   = chrdev;
        lock    = chrdevlk;
        break;
    default:
        printk("WARNING: Can't register file, not a device file\n");
        err = -ENXIO;
        goto error;
    }

    spin_lock(lock);
    forlinked(node, table[major], node->dev_next) {
        tail = node;
        if (node->dev_id.minor == dev->dev_id.minor) {
            err = -EEXIST;
            spin_unlock(lock);
            goto error;
        }
    }

    if (tail)
        tail->dev_next = dev;
    else table[major] = dev;

    dev->dev_prev = tail;
    dev->dev_next = NULL;

    if (dev->dev_probe) {
        if ((err = dev->dev_probe())) {
            spin_unlock(lock);
            goto error;
        }
    }

    spin_unlock(lock);
    return 0;
error:
    return err;
}

int kdev_unregister(struct devid *dd) {
    spinlock_t  *lock   = NULL;
    dev_t       *next   = NULL;
    dev_t       **table = NULL;

    if (dd == NULL)
        return -EINVAL;
    
    switch (dd->type) {
    case FS_CHR:
        table   = chrdev;
        lock    = chrdevlk;
        break;
    case FS_BLK:
        table   = blkdev;
        lock    = blkdevlk;
        break;
    default:
        return -ENXIO;
    }

    spin_lock(lock);

    forlinked(dev, table[dd->major], next) {
        // secure the next dev struct before removal of this dev.
        next = dev->dev_next;

        // check if this dev struct matched the dev_id.
        if (DEVID_CMP(&dev->dev_id, dd)) {
            // call the finalizer function on this device.
            if (dev->dev_fini != NULL)
                dev->dev_fini(dd);
            
            // remove the dev struct from the device table.
            if (dev->dev_prev != NULL)
                dev->dev_prev->dev_next = dev->dev_next;
            if (dev->dev_next != NULL)
                dev->dev_next->dev_prev = dev->dev_prev;
            break; // done.
        }
    }

    spin_unlock(lock);
    return 0;
}

int     kdev_close(struct devid *dd) {
    dev_t *dev = NULL;
    if (!(dev = kdev_get(dd)))
        return -ENXIO;
    
    if (!(dev->dev_ops.close))
        return -ENOSYS;
    return dev->dev_ops.close(dd);
}

int     kdev_open(struct devid *dd, inode_t **pip) {
    dev_t *dev = NULL;
    if (!(dev = kdev_get(dd)))
        return -ENXIO;
    
    if (!(dev->dev_ops.open))
        return -ENOSYS;
    return dev->dev_ops.open(dd, pip);
}

off_t   kdev_lseek(struct devid *dd, off_t off, int whence) {
    dev_t *dev = NULL;
    if (!(dev = kdev_get(dd)))
        return -ENXIO;
    
    if (!(dev->dev_ops.lseek))
        return -ENOSYS;
    return dev->dev_ops.lseek(dd, off, whence);
}

int     kdev_ioctl(struct devid *dd, int request, void *argp) {
    dev_t *dev = NULL;
    if (!(dev = kdev_get(dd)))
        return -ENXIO;
    
    if (!(dev->dev_ops.ioctl))
        return -ENOSYS;
    return dev->dev_ops.ioctl(dd, request, argp);
}

ssize_t kdev_read(struct devid *dd, off_t off, void *buf, size_t nbyte) {
    dev_t *dev = NULL;
    if (!(dev = kdev_get(dd)))
        return -ENXIO;
    
    if (!(dev->dev_ops.read))
        return -ENOSYS;
    return dev->dev_ops.read(dd, off, buf, nbyte);
}

ssize_t kdev_write(struct devid *dd, off_t off, void *buf, size_t nbyte) {
    dev_t *dev = NULL;

    // printk("%s:%d: %d->rdev[%d:%d]: %p\n",
        //    __FILE__, __LINE__, dd->type, dd->major, dd->minor, dd);

    if (!(dev = kdev_get(dd)))
        return -ENXIO;
    
    if (!(dev->dev_ops.write))
        return -ENOSYS;
    return dev->dev_ops.write(dd, off, buf, nbyte);
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
    
        if (!compare_strings(bdev_name, dev->dev_name)) {
            spin_unlock(blkdevlk);
            *pdev = dev->dev_id;
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

    if (NULL == dev->dev_ops.getinfo)
        return -ENOSYS;

    return dev->dev_ops.getinfo(dd, info);
}

int kdev_create(const char *dev_name, u8 type, u8 major, u8 minor, dev_t **pdev) {
    int     err     = 0;
    dev_t   *dev    = NULL;
    char    *name   = NULL;

    if (type != FS_BLK && type != FS_CHR)
        return -EINVAL;

    if ((dev = kdev_get(DEVID_PTR(type, DEV_T(major, minor)))))
        return -EEXIST;

    if (dev_name == NULL || pdev == NULL)
        return -EINVAL;

    if ((dev = (dev_t *)kmalloc(sizeof *dev)) == NULL)
        return -ENOMEM;
    
    if (NULL == (name = strdup(dev_name))) {
        kfree((void *)dev);
        return -ENOMEM;
    }

    memset(dev, 0, sizeof *dev);

    dev->dev_name = name;
    dev->dev_lck  = SPINLOCK_INIT();
    dev_lock(dev);

    dev->dev_id = DEVID(type, DEV_T(major, minor));
    *pdev = dev;

    return 0;
    if (dev) kfree(dev);
    return err;
}

void kdev_free(dev_t *dev) {
    if (dev == NULL)
        return;
    
    if (!dev_islocked(dev))
        dev_lock(dev);
    
    if (dev->dev_name)
        kfree(dev->dev_name);
    
    dev_unlock(dev);
    kfree(dev);
}

int kdev_mmap(struct devid *dd, vmr_t *region) {
    dev_t *dev = NULL;

    if (dd == NULL || region == NULL)
        return -EINVAL;
    
    if ((dev = kdev_get(dd)) == NULL)
        return -ENXIO;
    
    if (NULL == dev->dev_ops.mmap)
        return -ENOSYS;

    return dev->dev_ops.mmap(dd, region);
}