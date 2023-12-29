#include <lib/printk.h>
#include <sys/syscall.h>

void exit(int exit_code) {
    (void) exit_code;

    panic("%s:%ld: %s(%d);", __FILE__, __LINE__, __func__, exit_code);
}