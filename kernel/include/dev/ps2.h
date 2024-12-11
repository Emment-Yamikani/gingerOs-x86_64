#pragma once

#include <dev/dev.h>

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

// Wait until the input buffer is clear
static inline void ps2_wait_for_write(void) {
    while (inb(PS2_STATUS_PORT) & 0x02);
}

// Wait until there's data in the output buffer
static inline void ps2_wait_for_read(void) {
    while (!(inb(PS2_STATUS_PORT) & 0x01));
}

static inline void ps2_send_command(uint8_t command) {
    ps2_wait_for_write();
    outb(PS2_COMMAND_PORT, command);
}

static inline uint8_t ps2_read_data(void) {
    ps2_wait_for_read();
    return inb(PS2_DATA_PORT);
}

static inline void ps2_write_data(uint8_t data) {
    ps2_wait_for_write();
    outb(PS2_DATA_PORT, data);
}

static inline int ps2kbd_reset(void) {
    ps2_write_data(PS2_RESET);
    return ps2_read_data() == PS2_ACK;
}

static inline int ps2kbd_enable_scanning(void) {
    ps2_write_data(PS2_ENABLE_SCANNING);
    return ps2_read_data() == PS2_ACK;
}

static inline int ps2_self_test(void) {
    ps2_send_command(PS2_CMD_SELF_TEST);
    return ps2_read_data() == 0x55; // Controller self-test passes with 0x55
}

static inline int ps2_interface_test(void) {
    ps2_send_command(PS2_CMD_INTERFACE_TEST_PORT1);
    return ps2_read_data() == 0x00; // Interface test passes with 0x00
}


extern void ps2kbd_intr(void);