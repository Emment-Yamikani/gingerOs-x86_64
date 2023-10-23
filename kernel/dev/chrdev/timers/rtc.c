#include <bits/errno.h>
#include <dev/dev.h>
#include <dev/rtc.h>
#include <lib/printk.h>
#include <mm/kalloc.h>
#include <modules/module.h>
#include <arch/x86_64/system.h>
#include <lib/string.h>
#include <arch/chipset.h>
#include <arch/cpu.h>
#include <arch/traps.h>

#define RTC_CMD (0x70) // RTC command port.
#define RTC_IO  (0x71) // RTC io data port.

static spinlock_t *rtclk = SPINLOCK_NEW();
static dev_t rtcdev;

int rtc_probe();
int rtc_close(struct devid *dd);
int rtc_getinfo(struct devid *dd, void *info);
int rtc_open(struct devid *dd, int oflags, ...);
int rtc_ioctl(struct devid *dd, int req, void *argp);
off_t rtc_lseek(struct devid *dd, off_t off, int whence);
ssize_t rtc_read(struct devid *dd, off_t off, void *buf, size_t sz);
ssize_t rtc_write(struct devid *dd, off_t off, void *buf, size_t sz);

static dev_t rtcdev = {
    .devname = "rtc0",
    .devprobe = rtc_probe,
    .devlock = SPINLOCK_INIT(),
    .devid = {
        .minor = 0,
        .type = FS_CHR,
        .major = DEV_RTC0,
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
    pic_enable(IRQ_RTC);
    ioapic_enable(IRQ_RTC, cpuID);

    outb(RTC_CMD, 0x8A);
    uint8_t value = inb(RTC_IO);
    outb(RTC_CMD, 0x8A);
    outb(RTC_IO, ((value & 0xF0) | 0x0F));

    outb(RTC_CMD, 0x8B);
    value = inb(RTC_IO);
    outb(RTC_CMD, 0x8B);
    outb(RTC_IO, (value | 0x40));

    outb(RTC_CMD, 0x0C);
    inb(RTC_IO);
    return 0;
}

int rtc_open(struct devid *dd __unused, int oflags __unused, ...) {
    return 0;
}

int rtc_close(struct devid *dd __unused) {
    return 0;
}

int rtc_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOTSUP;
}

int rtc_ioctl(struct devid *dd, int req, void *argp __unused) {
    if (dd == NULL)
        return -EINVAL;
    
    switch (req) {
    case RTC_GETTOD:
        break;
    case RTC_SETTOD:
        break;
    case RTC_SETALM:
        break;
    case RTC_STOPALM:
        break;
    default:
        return -ENOTSUP;
    }

    return 0;
}

off_t rtc_lseek(struct devid *dd, off_t off, int whence) {
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

int rtc_init(void) {
    printk("Initializing Real Time Clock (RTC) timer...\n");
    return 0;
}

MODULE_INIT(rtcdev, NULL, rtc_init, NULL);