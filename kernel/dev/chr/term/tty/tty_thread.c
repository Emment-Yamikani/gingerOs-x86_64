#include <bits/errno.h>
#include <dev/tty.h>
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
 * TODO: Implement the VT100's terminal input logic.
 */
static void tty_input(void) {
    BUILTIN_THREAD_ANOUNCE(__func__);

    loop() {
        thread_yield();
    }
}

BUILTIN_THREAD(tty_input, (thread_entry_t)tty_input, NULL);