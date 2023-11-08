#include <dev/cga.h>
#include <dev/fb.h>
#include <bits/errno.h>
#include <dev/console.h>
#include <dev/uart.h>
#include <dev/limeterm.h>

int use_cga = 0;
int use_earlycons = 1;
int earlycons_use_gfx = 0;

int earlycons_init(void) {
    int err = 0;

    uart_dev_init();
    err = framebuffer_gfx_init();
    if (err == -ENOENT) {
        cga_init();
        use_cga = 1;
    }
    return 0;
}

void earlycons_usefb(void) {
    if (earlycons_use_gfx) {
        if (!limeterm_init())
            use_cga = 0;
    }
    else use_cga = 1;
}

int earlycons_putc(int c) {
    if (use_limeterm_cons) {
        limeterm_putc(c);
        return 0;
    } else if (earlycons_use_gfx) {
        limeterm_sputc(c);
        return 0;
    }
    cga_putc(c);
    return 0;
}