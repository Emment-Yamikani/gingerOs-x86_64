#include <dev/dev.h>
#include <mm/kalloc.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <bits/errno.h>
#include <boot/boot.h>
#include <sync/atomic.h>

typedef struct {
    void            *addr;
    char            *name;
    size_t          size;
    struct devid    dd;
    spinlock_t      lock;
} ramdisk_t;

static ramdisk_t    *ramdisks [256];
static int          ramdisk_cnt = 0;
static atomic_t     ramdisk_minor = {0};
static spinlock_t   *ramdisks_lk = &SPINLOCK_INIT();

#define ramdisks_lock()   ({ spin_lock(ramdisks_lk); })
#define ramdisks_unlock() ({ spin_unlock(ramdisks_lk); })

static int ramdisk_alloc_id(void) {
    int id = 0;
    
    spin_lock(ramdisks_lk);
    id = atomic_read(&ramdisk_minor);
    
    if (id >= (int)NELEM(ramdisks))
        goto error;
    
    atomic_inc(&ramdisk_minor);
    spin_unlock(ramdisks_lk);

    return id;
error:
    spin_unlock(ramdisks_lk);
    return -ENOMEM;
}

static int ramdisk_new(const char *name, ramdisk_t **ref) {
    int err = 0;
    int minor = 0;
    ramdisk_t *rd = NULL;

    if (!ref || !name)
        return -EINVAL;

    if (!(rd = kmalloc(sizeof *rd)))
        return -ENOMEM;

    memset(rd, 0, sizeof *rd);

    if (!(rd->name = strdup(name)))
        goto error;

    if ((err = minor = ramdisk_alloc_id()) < 0)
        goto error;

    rd->dd.minor = minor;
    rd->dd.major = DEV_RAMDISK;
    rd->lock = SPINLOCK_INIT();
    rd->dd.type = FS_BLK;

    *ref = rd;
    return 0;
error:
    if (rd)
        kfree(rd);
    if (rd->name)
        kfree(rd->name);
    
    return err;
}

static ramdisk_t *ramdisk_get(int minor) {
    ramdisk_t *rd = NULL;

    if (minor < 0 || minor > (int)NELEM(ramdisks))
        return NULL;

    ramdisks_lock();
    rd = ramdisks[minor];
    ramdisks_unlock();

    return rd;
}

int     ramdisk_close(struct devid *dd __unused) {
    return 0;
}

int     ramdisk_open(struct devid *dd __unused, int oflags __unused, ...) {
    return 0;
}

off_t   ramdisk_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused) {
    return -EINVAL;
}

int     ramdisk_ioctl(struct devid *dd __unused, int request __unused, void *argp __unused) {
    return -ENOTTY;
}

ssize_t ramdisk_read(struct devid *dd, off_t off, void *buf, size_t nbyte) {
    int locked = 0;
    ssize_t retval = 0;
    ramdisk_t *rd = NULL;

    if (!(rd = ramdisk_get(dd->minor)))
        return -ENOENT;

    if (!(locked = spin_islocked(&rd->lock)))
        spin_lock(&rd->lock);

    retval = MIN(rd->size - off, nbyte);
    memcpy(buf, rd->addr + off, retval);

    if (!locked)
        spin_unlock(&rd->lock);

    return retval;
}

ssize_t ramdisk_write(struct devid *dd, off_t off, void *buf, size_t nbyte) {
    int locked = 0;
    ssize_t retval = 0;
    ramdisk_t *rd = NULL;

    if (!(rd = ramdisk_get(dd->minor)))
        return -ENOENT;

    if (!(locked = spin_islocked(&rd->lock)))
        spin_lock(&rd->lock);

    retval = MIN(rd->size - off, nbyte);
    memcpy(rd->addr + off,  buf, retval);

    if (!locked)
        spin_unlock(&rd->lock);

    return retval;
}

static int ramdisk_mount(void) {
    int err = 0;
    ramdisk_t *rd = NULL;

    for (int i = 0; i < ramdisk_cnt; ++i) {
        if (!(rd = ramdisk_get(i)))
            continue;

        spin_lock(&rd->lock);
        
        spin_unlock(&rd->lock);
    }

    return 0;
    goto error;
error:
    return err;
}

static int ramdisk_getinfo(struct devid *dd, void *info) {
    int locked = 0;
    ramdisk_t *rd = NULL;
    bdev_info_t *bdev_info = info;

    if (dd == NULL || bdev_info == NULL)
        return -EINVAL;

    if (!(rd = ramdisk_get(dd->minor)))
        return -ENOENT;

    if (!(locked = spin_islocked(&rd->lock)))
        spin_lock(&rd->lock);
    
    *bdev_info = (bdev_info_t) {
        .bi_blocksize = 1,
        .bi_size = rd->size,
    };

    if (!locked)
        spin_unlock(&rd->lock);

    return 0;
}

static dev_t ramdiskdev = (dev_t) {
    .devlock = {0},
    .devname = "ramdisk",
    .devprobe = NULL,
    .devmount = ramdisk_mount,
    .devid = {
        .minor = 0,
        .type = FS_BLK,
        .major = DEV_RAMDISK,
    },
    .devops = {
        .open = ramdisk_open,
        .read = ramdisk_read,
        .write = ramdisk_write,
        .lseek = ramdisk_lseek,
        .ioctl = ramdisk_ioctl,
        .close = ramdisk_close,
        .getinfo = ramdisk_getinfo,
    },
};

static int ramdisk_init(void) {
    int err = 0;
    ramdisk_t *rd = NULL;

    printk("initializing ramdisks...\n");

    memset(ramdisks, 0, sizeof ramdisks);

    for (size_t i = 0; i < bootinfo.modcnt; ++i) {
        if ((err = ramdisk_new(bootinfo.mods[i].cmd, &rd)))
            return err;
        
        rd->size = bootinfo.mods[i].size;
        rd->addr = (void *)bootinfo.mods[i].addr;

        ramdisks_lock();
        ramdisk_cnt++;
        ramdisks[rd->dd.minor] = rd;
        ramdisks_unlock();

        printk("\'%s\' ramdisk installed\n", rd->name);
    }

    if ((err = kdev_register(&ramdiskdev, DEV_RAMDISK, FS_BLK)))
        return err;

    return 0;
}

MODULE_INIT(ramdiskdev, NULL, ramdisk_init, NULL);