#include <bits/errno.h>
#include <boot/boot.h>
#include <dev/dev.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <sync/atomic.h>

#define NRAMDISK    64

#define RAMDISK_GETINFO 0   // ioctl command to get ramdisk info.

typedef struct {
    int         rd_id;
    void        *rd_addr;
    dev_t       *rd_dev;
    usize       rd_size;
    spinlock_t  rd_lock;
} ramdisk_t;

#define ramdisk_lock(rd)             ({ spin_lock(&(rd)->rd_lock); })
#define ramdisk_unlock(rd)           ({ spin_unlock(&(rd)->rd_lock); })
#define ramdisk_islocked(rd)         ({ spin_islocked(&(rd)->rd_lock); })
#define ramdisk_assert_locked(rd)    ({ spin_assert_locked(&(rd)->rd_lock); })

static atomic_t  ramdisk_count = 0;     // ramdisk count.
static ramdisk_t *ramdisks[NRAMDISK];   // ramdisk table.
static SPINLOCK(ramdisks_lk);           // spinlock for ramdisk table.

#define ramdisks_lock()             ({ spin_lock(ramdisks_lk); })
#define ramdisks_unlock()           ({ spin_unlock(ramdisks_lk); })
#define ramdisks_islocked()         ({ spin_islocked(ramdisks_lk); })
#define ramdisks_assert_locked()    ({ spin_assert_locked(ramdisks_lk); })

// declare device operations for ramdisk.
DEV_DECL_OPS(static, ramdisk);

static int ramdisk_probe(void) {
    return 0;
}

static void ramdisk_fini(struct devid *dd __unused) {
    printk("%s:%d: %s()\n", __FILE__, __LINE__, __func__);
}

/// allocate a ramdisk instance.
/// these ramdisks may be modules passed by the bootloader.
static int ramdisk_alloc(ramdisk_t **ref) {
    ramdisk_t *ramdisk = NULL;

    if (ref == NULL)
        return -EINVAL;

    if (NULL == (ramdisk = (ramdisk_t *)kmalloc(sizeof *ramdisk)))
        return -ENOMEM;

    memset(ramdisk, 0, sizeof *ramdisk);
    
    ramdisk->rd_lock= SPINLOCK_INIT();
    ramdisk_lock(ramdisk);

    *ref = ramdisk;
    return 0;
}

static int ramdisk_init(void) {
    char        name[16];
    int         err      = 0;
    mod_t       *mod     = NULL;
    dev_t       *dev     = NULL;
    ramdisk_t   *ramdisk = NULL;

    mod     = (mod_t *)bootinfo.mods; // get the first module in the array.

    /// Do not allow more than what the kernel expects for
    /// No. of ramdisks.
    if (bootinfo.modcnt > NRAMDISK)
        return -EAGAIN;

    for (usize i = 0; i < bootinfo.modcnt; ++i, mod++) {
        if ((err = ramdisk_alloc(&ramdisk)))
            return err;

        snprintf(name, sizeof name, "ramdisk%d", i);

        if ((err = kdev_create(name, FS_BLK, DEV_RAMDISK, i, &dev)))
            return err;
        
        dev->dev_fini   = ramdisk_fini;
        dev->dev_probe  = ramdisk_probe;
        dev->dev_priv   = (void *)ramdisk;
        dev->dev_ops    = DEVOPS(ramdisk);

        printk("Initializing '\e[025453;011m%s\e[0m' chardev...\n", dev->dev_name);

        if ((err = kdev_register(dev, DEV_RAMDISK, FS_BLK))) {
            printk("%s:%d: failed to register '%s'\n",
                __FILE__, __LINE__, dev->dev_name);
            kdev_free(dev);
            return err;
        }

        ramdisk->rd_dev = dev;        
        ramdisk->rd_size= mod->size;
        ramdisk->rd_addr= (void *)mod->addr;
        ramdisk->rd_id  = dev->dev_id.minor; // set ramdisk id.

        atomic_inc(&ramdisk_count);

        ramdisks[i] = ramdisk;

        ramdisk_unlock(ramdisk);
        dev_unlock(dev);
    }

    return 0;
} MODULE_INIT(ramdisk, NULL, ramdisk_init, NULL);

static int ramdisk_get(int minor, ramdisk_t **ref) {
    if (ref == NULL)
        return -EINVAL;

    if ((minor >= NRAMDISK) || (minor < 0) ||
        (minor >= (int)atomic_read(&ramdisk_count)))
        return -EINVAL;

    ramdisks_lock();
    *ref = ramdisks[minor];
    ramdisks_unlock();

    return 0;
}

static int ramdisk_close(struct devid *dd __unused) {
    return 0;
}

static int ramdisk_open(struct devid *dd __unused, inode_t **pip __unused) {
    return 0;
}

static int ramdisk_mmap(struct devid *dd __unused, vmr_t *region __unused) {
    return -ENOSYS;
}

static int ramdisk_getinfo(struct devid *dd, void *info) {
    int         err         = 0;
    int         locked      = 0;
    ramdisk_t   *ramdisk    = NULL;
    bdev_info_t *bdev_info  = (bdev_info_t *)info;

    if (bdev_info == NULL)
        return -EINVAL;

    if (dd->major != DEV_RAMDISK)
        return -EINVAL;

    if ((err = ramdisk_get(dd->minor, &ramdisk)))
        return err;

    if ((locked = !ramdisk_islocked(ramdisk)))
        ramdisk_lock(ramdisk);

    *bdev_info = (bdev_info_t) {
        .bi_blocksize = 1,
        .bi_size      = ramdisk->rd_size
    };

    if (locked != 0)
        ramdisk_unlock(ramdisk);

    return 0;
}

static off_t ramdisk_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused) {
    return -ENOSYS;
}

static int ramdisk_ioctl(struct devid *dd, int request, void *argp) {
    int         err         = 0;
    int         locked      = 0;
    ramdisk_t   *ramdisk    = NULL;
    bdev_info_t *bdev_info  = (bdev_info_t *)argp;

    if (bdev_info == NULL)
        return -EINVAL;

    if (dd->major != DEV_RAMDISK)
        return -EINVAL;

    if ((err = ramdisk_get(dd->minor, &ramdisk)))
        return err;

    if ((locked = !ramdisk_islocked(ramdisk)))
        ramdisk_lock(ramdisk);
    
    switch(request) {
    case RAMDISK_GETINFO:
        *bdev_info = (bdev_info_t) {
            .bi_blocksize = 1,
            .bi_size      = ramdisk->rd_size
        };
        break;
    default:
        err = -EINVAL;
    }

    if (locked != 0)
        ramdisk_unlock(ramdisk);
    return err;
}

static ssize_t ramdisk_read(struct devid *dd, off_t off, void *buf, size_t nbyte) {
    int         err         = 0;
    int         locked      = 0;
    isize       retval      = 0;
    ramdisk_t   *ramdisk    = NULL;

    if (buf == NULL)
        return -EINVAL;

    if (dd->major != DEV_RAMDISK)
        return -EINVAL;

    if ((err = ramdisk_get(dd->minor, &ramdisk)))
        return err;

    if ((locked = !ramdisk_islocked(ramdisk)))
        ramdisk_lock(ramdisk);
    
    retval = MIN(ramdisk->rd_size - off, nbyte);
    memcpy(buf, ramdisk->rd_addr + off, retval);

    if (locked != 0)
        ramdisk_unlock(ramdisk);
    return retval;
}

static ssize_t ramdisk_write(struct devid *dd, off_t off, void *buf, size_t nbyte) {
    int         err         = 0;
    int         locked      = 0;
    isize       retval      = 0;
    ramdisk_t   *ramdisk    = NULL;

    if (buf == NULL)
        return -EINVAL;

    if (dd->major != DEV_RAMDISK)
        return -EINVAL;

    if ((err = ramdisk_get(dd->minor, &ramdisk)))
        return err;

    if ((locked = !ramdisk_islocked(ramdisk)))
        ramdisk_lock(ramdisk);
    
    retval = MIN(ramdisk->rd_size - off, nbyte);
    memcpy(ramdisk->rd_addr + off,  buf, retval);

    if (locked != 0)
        ramdisk_unlock(ramdisk);
    return retval;
}