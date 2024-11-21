#include <dev/dev.h>
#include <boot/boot.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <lib/printk.h>
#include <mm/kalloc.h>
#include <modules/module.h>
#include <dev/console.h>
#include <dev/uart.h>
#include <dev/limeterm.h>

DEV_DECL_OPS(static, console);
static DEV_INIT(console, FS_CHR, DEV_CONSOLE, 1);

static int use_fb = 0;

static int console_probe(void) {
    return 0;
}

static int console_init(void) {
    printk("Initializing \e[025453;011m%s\e[0m chardev...\n", consoledev.dev_name);
    if (bootinfo.fb.type == 1)
        use_fb = 1;

    if (use_fb)
        printk("console will use framebuffer\n");

    return kdev_register(&consoledev, DEV_CONSOLE, FS_CHR);
}

int console_putc(int c) {
    uart_putc(c);
    if (use_earlycons)
        return earlycons_putc(c);
    return 0;
}

static int console_open(struct devid *dd __unused, inode_t **pip __unused) {
    return 0;
}

static int console_close(struct devid *dd __unused) {
    return 0;
}

static int console_ioctl(struct devid *dd __unused, int req __unused, void *argp __unused) {
    return 0;
}

static isize console_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, usize sz __unused) {
    for (char *c = buf; sz--; ++c) {
        console_putc(*c);
    }
    return 0;
}

static isize console_read(struct devid *dd __unused, off_t off __unused, void *buf __unused, usize sz __unused) {
    return 0;
}

static int console_mmap(struct devid *dd __unused, vmr_t *vmr __unused) {
    return -EOPNOTSUPP;
}

static int console_getinfo(struct devid *dd __unused, void *info __unused) {
    return -EOPNOTSUPP;
}

static off_t console_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused) {
    return -EOPNOTSUPP;
}

MODULE_INIT(console, NULL, console_init, NULL);