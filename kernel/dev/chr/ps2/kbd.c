#include <arch/x86_64/system.h>
#include <arch/chipset.h>
#include <bits/errno.h>
#include <dev/dev.h>
#include <arch/traps.h>
#include <ds/ringbuf.h>
#include <dev/ps2.h>

DEV_DECL_OPS(static, ps2kbd);

static DEV_INIT(ps2kbd, FS_CHR, DEV_KBD0, 0);

// Ring buffer for scancodes
static ringbuf_t kbd_buffer;

#define buffer_lock()               ({ ringbuf_lock(&kbd_buffer); })
#define buffer_unlock()             ({ ringbuf_unlock(&kbd_buffer); })
#define buffer_islocked()           ({ ringbuf_islocked(&kbd_buffer); })
#define buffer_asser_locked()       ({ ringbuf_asser_locked(&kbd_buffer); })


void ps2kbd_intr(void) {
    uint8_t scancode = inb(PS2_DATA_PORT);

    buffer_lock();
    ringbuf_write(&kbd_buffer, (void *)&scancode, 1);
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
    if ((err = ringbuf_init(PGSZ, &kbd_buffer)))
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

int ps2kbd_init(void) {
    printk("Initializing \e[025453;011m%s\e[0m chardev...\n", ps2kbddev.dev_name);
    return kdev_register(&ps2kbddev, DEV_KBD0, FS_CHR);
}

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
    return -ENOSYS;
}

static ssize_t ps2kbd_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t nbyte __unused) {
    return -ENOSYS;
}