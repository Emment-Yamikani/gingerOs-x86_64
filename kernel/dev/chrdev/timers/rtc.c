#include <dev/dev.h>
#include <modules/module.h>
#include <bits/errno.h>
#include <lib/printk.h>
#include <mm/kalloc.h>
#include <arch/x86_64/system.h>

#define RTC_CMD (0x70) // RTC command port.
#define RTC_IO  (0x71) // RTC io data port.

static spinlock_t *rtclk = SPINLOCK_NEW();
static dev_t rtcdev;

int rtc_probe();
int rtc_open(struct devid *dd);
int rtc_close(struct devid *dd);
int rtc_getinfo(struct devid *dd, void *info);
int rtc_ioctl(struct devid *dd, int req, void *argp);
int rtc_lseek(struct devid *dd, off_t off, int whence);
ssize_t rtc_read(struct devid *dd, off_t off, void *buf, size_t sz);
ssize_t rtc_write(struct devid *dd, off_t off, void *buf, size_t sz);


int rtc_init(void) {
    printk("Initializing Real Time Clock (RTC) timer...\n");
    return 0;
}
MODULE_INIT(rtc0, NULL, rtc_init, NULL);

static dev_t rtcdev = {
    .devname = "rtc0",
    .devprobe = rtc_probe,
    .devlock = SPINLOCK_INIT(),
    .devid = {
        .major = DEV_RTC0,
        .minor = 0,
        .type = FS_CHR,
    },
    .devops = {
        .close = rtc_close,
        .getinfo = rtc_getinfo,
        .ioctl = rtc_ioctl,
        .lseek = rtc_lseek,
        .open = rtc_open,
        .read = rtc_read,
        .write = rtc_write,
    },
};

int rtc_probe() {
    return -ENOTSUP;
}

int rtc_open(struct devid *dd) {
    return -ENOTSUP;
}

int rtc_close(struct devid *dd) {
    return -ENOTSUP;
}

int rtc_getinfo(struct devid *dd, void *info) {
    return -ENOTSUP;
}

int rtc_ioctl(struct devid *dd, int req, void *argp) {
    return -ENOTSUP;
}

int rtc_lseek(struct devid *dd, off_t off, int whence) {
    (void)dd;
    (void)off;
    (void)whence;
    return -ENOTSUP;
}

ssize_t rtc_read(struct devid *dd, off_t off, void *buf, size_t sz) {
    (void)dd;
    (void)off;
    (void)buf;
    (void)sz;
    return -ENOTSUP;
}

ssize_t rtc_write(struct devid *dd, off_t off, void *buf, size_t sz) {
    (void)dd;
    (void)off;
    (void)buf;
    (void)sz;
    return -ENOTSUP;
}