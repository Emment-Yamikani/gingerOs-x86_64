#include <fs/fs.h>
#include <dev/dev.h>
#include <sys/system.h>
#include <lib/string.h>
#include <arch/x86_64/system.h>
#include <sync/spinlock.h>
#include <bits/errno.h>
#include <modules/module.h>
#include <lib/printk.h>

volatile int use_uart = 0;

struct dev uartdev;

#define SERIAL_MMIO 0
#define SERIAL_PORT 0x3F8

static spinlock_t *uartlock = SPINLOCK_NEW();

#define UART_IER    1
#define UART_FCR    2
#define UART_LCR    3
#define UART_MCR    4
#define UART_DLL    0
#define UART_DLH    1
#define UART_LCR_DLAB    0x80

void serial_init() {
    outb(SERIAL_PORT + UART_IER, 0x00);    /* Disable all interrupts */
    outb(SERIAL_PORT + UART_FCR, 0x00);    /* Disable FIFO */
    outb(SERIAL_PORT + UART_LCR, 0x03);    /* 8 bits, no parity, one stop bit */
    outb(SERIAL_PORT + UART_MCR, 0x03);    /* RTS + DTR */

    uint8_t lcr = inb(SERIAL_PORT + UART_LCR);
    outb(SERIAL_PORT + UART_LCR, lcr | UART_LCR_DLAB);  /* Enable DLAB */
    outb(SERIAL_PORT + UART_DLL, 0x18);    /* Set divisor to 1 (lo byte) 115200 baud */
    outb(SERIAL_PORT + UART_DLH, 0x00);    /*                  (hi byte)            */
    outb(SERIAL_PORT + UART_LCR, lcr & ~UART_LCR_DLAB); /* Disable DLAB */
}

int uart_tx_empty() {
    return inb(SERIAL_PORT + 5) & 0x20;
}

int serial_chr(char chr) {
    if (chr == '\n')
        serial_chr('\r');

    while (!uart_tx_empty());

    outb(SERIAL_PORT + 0, chr);

    return 1;
}

int serial_str(char *str) {
    int ret = 0;

    while (*str) {
        ++ret;
        ret += serial_chr(*str++);
    }

    return ret;
}

int uart_puts(char *s) {
    return serial_str(s);
}

int _uart_putc(char c) {
    return serial_chr(c);
}

void uart_init() {
#if SERIAL_MMIO
    pmman.map_to(0x9000B000, 0xCF00B000, PAGE_SIZE, VM_KRW);
#endif
    serial_init();

    /* Assume a terminal, clear formatting */
    serial_str("\e[0m\n");
}

ssize_t uart_write(struct devid *dd __unused, off_t off __unused, void *buf, size_t sz) {
    char *buff = (char *)buf;
    //no out of order console writing
    spin_lock(uartlock);
    size_t size = sz;
    while(sz--)
        _uart_putc(*buff++);
    spin_unlock(uartlock);
    return size;
}

int uart_putc(char c) {
    return (int)uart_write(NULL, 0, (char *)&c, 1);
}

size_t uart_read(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz __unused) { //no out of order console writing
    return 0;
}

int uart_open(struct devid *dd __unused, int mode __unused, ...) {
    return 0;
}

int uart_ioctl(struct devid *dd __unused, int req __unused, void *argp __unused) {
    return -EINVAL;
}

int uart_close(struct devid *dd __unused) {
    return -EINVAL;
}

int uart_dev_init(void) {
    uart_init();
    use_uart = 1;
    return 0;
}

int uart_probe() {
    return 0;
}