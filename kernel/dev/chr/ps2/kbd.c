#include <arch/x86_64/system.h>
#include <arch/chipset.h>
#include <bits/errno.h>
#include <dev/dev.h>
#include <arch/traps.h>
#include <ds/ringbuf.h>


DEV_DECL_OPS(static, ps2kbd);

static DEV_INIT(ps2kbd, FS_CHR, DEV_KBD0, 0);

#define PS2_DATA_PORT                   0x60
#define PS2_STATUS_PORT                 0x64
#define PS2_COMMAND_PORT                0x64

#define PS2_CMD_DISABLE_PORT1           0xAD
#define PS2_CMD_ENABLE_PORT1            0xAE
#define PS2_CMD_SELF_TEST               0xAA
#define PS2_CMD_INTERFACE_TEST_PORT1    0xAB

#define PS2_RESET                       0xFF
#define PS2_ACK                         0xFA
#define PS2_ENABLE_SCANNING             0xF4

// Ring buffer for scancodes
static ringbuf_t keyboard_buffer;

#define buffer_lock()               ({ ringbuf_lock(&keyboard_buffer); })
#define buffer_unlock()             ({ ringbuf_unlock(&keyboard_buffer); })
#define buffer_islocked()           ({ ringbuf_islocked(&keyboard_buffer); })
#define buffer_asser_locked()       ({ ringbuf_asser_locked(&keyboard_buffer); })

// Wait until the input buffer is clear
static inline void ps2_wait_for_write(void) {
    while (inb(PS2_STATUS_PORT) & 0x02);
}

// Wait until there's data in the output buffer
static inline void ps2_wait_for_read(void) {
    while (!(inb(PS2_STATUS_PORT) & 0x01));
}

void ps2_send_command(uint8_t command) {
    ps2_wait_for_write();
    outb(PS2_COMMAND_PORT, command);
}

uint8_t ps2_read_data(void) {
    ps2_wait_for_read();
    return inb(PS2_DATA_PORT);
}

void ps2_write_data(uint8_t data) {
    ps2_wait_for_write();
    outb(PS2_DATA_PORT, data);
}

int ps2kbd_reset(void) {
    ps2_write_data(PS2_RESET);
    return ps2_read_data() == PS2_ACK;
}

int ps2kbd_enable_scanning(void) {
    ps2_write_data(PS2_ENABLE_SCANNING);
    return ps2_read_data() == PS2_ACK;
}

int ps2_self_test(void) {
    ps2_send_command(PS2_CMD_SELF_TEST);
    return ps2_read_data() == 0x55; // Controller self-test passes with 0x55
}

int ps2_interface_test(void) {
    ps2_send_command(PS2_CMD_INTERFACE_TEST_PORT1);
    return ps2_read_data() == 0x00; // Interface test passes with 0x00
}

void ps2kbd_intr(void) {
    uint8_t scancode = inb(PS2_DATA_PORT);

    buffer_lock();
    ringbuf_write(&keyboard_buffer, (void *)&scancode, 1);
    buffer_unlock();
}

static int ps2kbd_probe(void) {
    int err = 0;

    // Disable PS/2 port 1
    ps2_send_command(PS2_CMD_DISABLE_PORT1);

    // Perform controller self-test
    if (!ps2_self_test()) {
        return -EINVAL;
    }

    // Perform interface test for port 1
    if (!ps2_interface_test()) {
        return -EINVAL;
    }

    // Re-enable PS/2 port 1
    ps2_send_command(PS2_CMD_ENABLE_PORT1);

    // Initialize the ring buffer
    if ((err = ringbuf_init(PGSZ, &keyboard_buffer)))
        return err;

    // Reset the keyboard
    if (!ps2kbd_reset()) {
        return -EINVAL;
    }

    // Enable keyboard scanning
    if (!ps2kbd_enable_scanning()) {
        return -EINVAL;
    }

    // enable interrupt on line 1.
    pic_enable(PS2_KBD);
    ioapic_enable(PS2_KBD, getcpuid());

    return 0;
}

static int ps2kbd_init(void) {
    printk("Initializing \e[025453;011m%s\e[0m chardev...\n", ps2kbddev.dev_name);
    return kdev_register(&ps2kbddev, DEV_KBD0, FS_CHR);
} MODULE_INIT(ps2kbd, NULL, ps2kbd_init, 0ULL);

static int ps2kbd_close(struct devid *dd __unused) {
    return 0;
}

static int ps2kbd_open(struct devid *dd __unused, inode_t **pip __unused) {
    return 0;
}

static int ps2kbd_mmap(struct devid *dd __unused, vmr_t *region __unused) {
    return -ENOSYS;
}

static int ps2kbd_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOSYS;
}

static off_t ps2kbd_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused __unused) {
    return -ENOSYS;
}

static int ps2kbd_ioctl(struct devid *dd __unused, int request __unused, void *arg __unused __unused) {
    return -ENOSYS;
}

static ssize_t ps2kbd_read(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t nbyte __unused) {
    printk("%s:%d: %s(%d, %p, %d);\n", __FILE__, __LINE__, __func__, dd->minor, buf, nbyte);
    return 0;
}

static ssize_t ps2kbd_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t nbyte __unused) {
    printk("%s:%d: %s(%d, %p, %d);\n", __FILE__, __LINE__, __func__, dd->minor, buf, nbyte);
    return 0;
}