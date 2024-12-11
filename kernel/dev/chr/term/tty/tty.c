#include <bits/errno.h>
#include <dev/tty.h>
#include <lib/string.h>
#include <lib/printk.h>
#include <sys/thread.h>

DEV_DECL_OPS(static, tty);

static tty_t ttys[NTTY];

static int tty_probe(void) {
    return 0;
}

static int tty_init(void) {
    int     err     = 0;
    dev_t   *dev    = NULL;
    char    tty_name[8];

    memset(ttys, 0, sizeof ttys);

    for (int tty = 0; tty < NTTY; ++tty) {
        snprintf(tty_name, sizeof tty_name, "tty%d", tty);

        if ((err = kdev_create(tty_name, FS_CHR, DEV_TTY, tty, &dev)))
            return err;

        ttys[tty].dev   = dev;
        dev->dev_probe  = tty_probe;
        dev->dev_ops    = DEVOPS(tty);

        printk("Initializing \e[025453;011m%s\e[0m chardev...\n", dev->dev_name);

        if ((err = kdev_register(dev, DEV_TTY, FS_CHR))) {
            for (int tty = 0; tty < NTTY; ++tty) {
                int     err = 0;
    
                if ((err = kdev_unregister(DEVID_PTR(FS_CHR, DEV_T(DEV_TTY, tty))))) {
                    printk("%s:%d: Error unregistering device, err: %d\n",
                        __FILE__, __LINE__, err);

                    // FIXME: is continuing the bes thing to do here or freeing dev?
                    continue;
                }
                kdev_free(ttys[tty].dev);
            }

            return err;
        }
        dev_unlock(dev); // dev was locked in kdev_create();
    }

    return 0;
} MODULE_INIT(tty, tty_init, NULL, NULL);

static int     tty_close(struct devid *dd __unused) {
    return 0;
}

static int     tty_open(struct devid *dd __unused, inode_t **pip __unused) {
    return 0;
}

static int     tty_mmap(struct devid *dd __unused, vmr_t *region __unused) {
    return -ENOSYS;
}

static int     tty_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOSYS;
}

static off_t   tty_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused __unused) {
    return -ENOSYS;
}

static int     tty_ioctl(struct devid *dd __unused, int request __unused, void *arg __unused __unused) {
    return -ENOSYS;
}

static ssize_t tty_read(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t nbyte __unused) {
    printk("%s:%d: %s(%d, %p, %d);\n", __FILE__, __LINE__, __func__, dd->minor, buf, nbyte);
    return 0;
}

static ssize_t tty_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t nbyte __unused) {
    printk("%s:%d: %s(%d, %p, %d);\n", __FILE__, __LINE__, __func__, dd->minor, buf, nbyte);
    return 0;
}