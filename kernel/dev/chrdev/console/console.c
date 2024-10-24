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

dev_t consoledev;
static int use_fb = 0;

static int console_init(void) {
    printk("Initializing \e[025453;011m%s\e[0m chardev...\n", "console");
    if (bootinfo.fb.type == 1)
        use_fb = 1;

    if (use_fb)
        printk("console will use framebuffer\n");
    return 0;
}

int console_putc(int c) {
    uart_putc(c);
    if (use_earlycons)
        return earlycons_putc(c);
    return 0;
}

MODULE_INIT(console, NULL, console_init, NULL);