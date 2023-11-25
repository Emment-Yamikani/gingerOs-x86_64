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
#include <sync/cond.h>
#include <arch/firmware/acpi.h>

int rtc_probe();
int rtc_close(struct devid *dd);
int rtc_getinfo(struct devid *dd, void *info);
int rtc_open(struct devid *dd, int oflags, ...);
int rtc_ioctl(struct devid *dd, int req, void *argp);
off_t rtc_lseek(struct devid *dd, off_t off, int whence);
ssize_t rtc_read(struct devid *dd, off_t off, void *buf, size_t sz);
ssize_t rtc_write(struct devid *dd, off_t off, void *buf, size_t sz);

#define RTC_CMD (0x70)  // RTC command port.
#define RTC_IO  (0x71)  // RTC io data port.

#define RTC_SEC 0x00 // Seconds       0-59
#define RTC_MIN 0x02 // Minutes       0-59
#define RTC_HRS 0x04 // Hours         0-23? 1-12

// #define RTC_DOW 0x06 // Day of Week   1-7
#define RTC_DAY 0x07 // Day of Month  1-31
#define RTC_MON 0x08 // Month of Year 1-12
#define RTC_YR  0x09 // Year          0-99

// Status register A
#define RTC_STA 0x0A
// Status register B
#define RTC_STB 0x0B

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

#define CURRENT_YEAR    2023

static dev_t rtcdev;
static size_t rtc_secs = 0;
static size_t rtc_ticks = 0;
static uint16_t RTC_CENT = 0;
static rtc_time_t rtc_tm = {0};
static cond_t *rtc_event = COND_NEW();
static spinlock_t *rtclk = SPINLOCK_NEW();

static int rtc_updating(void) {
    outb(RTC_CMD, RTC_STA);
    return (inb(RTC_IO) & 0x80);
}

static int rtc_gettime(rtc_time_t *tm) {
    uint8_t statusB = 0;
    rtc_time_t last = {0};

    if (tm == NULL)
        return -EINVAL;
    

    while (rtc_updating());
    outb(RTC_CMD, RTC_SEC);
    tm->rtc_sec = inb(RTC_IO);
    outb(RTC_CMD, RTC_MIN);
    tm->rtc_min = inb(RTC_IO);
    outb(RTC_CMD, RTC_HRS);
    tm->rtc_hrs = inb(RTC_IO);
    outb(RTC_CMD, RTC_DAY);
    tm->rtc_day = inb(RTC_IO);
    outb(RTC_CMD, RTC_MON);
    tm->rtc_mon = inb(RTC_IO);
    outb(RTC_CMD, RTC_YR);
    tm->rtc_year = inb(RTC_IO);
    if (RTC_CENT) {
        outb(RTC_CMD, RTC_CENT);
        tm->rtc_cent = inb(RTC_IO);
    }

    do {
        last = *tm;

        while (rtc_updating());
        outb(RTC_CMD, RTC_SEC);
        tm->rtc_sec = inb(RTC_IO);
        outb(RTC_CMD, RTC_MIN);
        tm->rtc_min = inb(RTC_IO);
        outb(RTC_CMD, RTC_HRS);
        tm->rtc_hrs = inb(RTC_IO);
        outb(RTC_CMD, RTC_DAY);
        tm->rtc_day = inb(RTC_IO);
        outb(RTC_CMD, RTC_MON);
        tm->rtc_mon = inb(RTC_IO);
        outb(RTC_CMD, RTC_YR);
        tm->rtc_year = inb(RTC_IO);
        if (RTC_CENT != 0) {
            outb(RTC_CMD, RTC_CENT);
            tm->rtc_cent = inb(RTC_IO);
        }
    } while ((last.rtc_sec != tm->rtc_sec) ||
             (last.rtc_min != tm->rtc_min) ||
             (last.rtc_hrs != tm->rtc_hrs) ||
             (last.rtc_day != tm->rtc_day) ||
             (last.rtc_mon != tm->rtc_mon) ||
             (last.rtc_year != tm->rtc_year) ||
             (last.rtc_cent != tm->rtc_cent));
    
    outb(RTC_CMD, RTC_STB);
    statusB = inb(RTC_IO);

    if ((statusB & 0x4) == 0) {
        tm->rtc_sec     = BCD2binary(tm->rtc_sec);
        tm->rtc_min     = BCD2binary(tm->rtc_min);
        tm->rtc_hrs     = BCD2binary(tm->rtc_hrs) | (tm->rtc_hrs & 0x80);
        tm->rtc_day     = BCD2binary(tm->rtc_day);
        tm->rtc_mon     = BCD2binary(tm->rtc_mon);
        tm->rtc_year    = BCD2binary(tm->rtc_year);
        if (RTC_CENT != 0)
            tm->rtc_cent    = BCD2binary(tm->rtc_cent);
    }

    // Convert 12 hour clock to 24 hour clock if necessary
 
      if (!(statusB & 0x02) && (tm->rtc_hrs & 0x80)) {
            tm->rtc_hrs = ((tm->rtc_hrs & 0x7F) + 12) % 24;
      }
 
      // Calculate the full (4-digit) year
 
      if(RTC_CENT != 0) {
            tm->rtc_year += tm->rtc_cent * 100;
      } else {
            tm->rtc_year += (CURRENT_YEAR / 100) * 100;
            if(tm->rtc_year < CURRENT_YEAR) tm->rtc_year += 100;
      }
    return 0;
}

int rtc_probe(void) {
    acpiFADT_t *FADT = (acpiFADT_t *)acpi_enumerate("FACP");
    RTC_CENT = FADT ? FADT->RTC_CENTURY: 0;

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

    rtc_gettime(&rtc_tm);

    printk("Time: \e[025453;02m%d:%d:%d\e[0m Date: \e[025453;03m%d/%d/%d\e[0m\n",
           rtc_tm.rtc_hrs, rtc_tm.rtc_min, rtc_tm.rtc_sec,
           rtc_tm.rtc_day, rtc_tm.rtc_mon, rtc_tm.rtc_year);
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
    int err = 0;

    if (dd == NULL)
        return -EINVAL;

    spin_lock(rtclk);
    switch (req)
    {
    case RTC_GETTIME:
        err = rtc_gettime(argp);
        break;
    case RTC_SETTIME:
        err = -ENOTSUP;
        break;
    case RTC_SETALM:
        err = -ENOTSUP;
        break;
    default:
        err = -ENOTSUP;
    }

    spin_lock(rtclk);
    return err;
}

off_t rtc_lseek(struct devid *dd, off_t off, int whence)
{
    (void)dd;
    (void)off;
    (void)whence;
    return -ENOTSUP;
}

ssize_t rtc_read(struct devid *dd, off_t off, void *buf, size_t sz)
{
    (void)dd;
    (void)off;
    (void)buf;
    (void)sz;
    return -ENOTSUP;
}

ssize_t rtc_write(struct devid *dd, off_t off, void *buf, size_t sz)
{
    (void)dd;
    (void)off;
    (void)buf;
    (void)sz;
    return -ENOTSUP;
}

void rtc_intr(void) {

    spin_lock(rtclk);
    if (!((++rtc_ticks) % 2)) {
        ++rtc_secs;
        if ((rtc_secs % 60) == 0)
            rtc_gettime(&rtc_tm);
        cond_broadcast(rtc_event);
    }

    outb(RTC_CMD, 0x0C);
    inb(RTC_IO);
    spin_unlock(rtclk);
}

int rtc_init(void) {
    printk("Initializing Real Time Clock (RTC) timer...\n");
    return kdev_register(&rtcdev, DEV_RTC0, FS_CHR);
}

MODULE_INIT(rtcdev, NULL, rtc_init, NULL);