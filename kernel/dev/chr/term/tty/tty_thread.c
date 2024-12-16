#include <bits/errno.h>
#include <dev/tty.h>
#include <fs/file.h>
#include <lib/printk.h>
#include <mm/kalloc.h>
#include <sys/thread.h>

/**
 * @brief This is a thread handler for the keyboard input.
 * this will be used for the current tty input.
 * 
 * When a key is pressed or released at the keyboard.
 * this thread will read, process the scan code if need be and
 * the store it into the current tty's input buffer.
 * TODO: Implement the VT100's terminal input logic. */
static void tty_input(void) {
    char    ch;

    loop() {
        kdev_read(DEVID_PTR(FS_CHR, DEV_T(DEV_KBD0, 0)), 0, &ch, sizeof ch);
        printk("%x;\n", ch);
    }
} BUILTIN_THREAD(tty_input, (thread_entry_t)tty_input, NULL);