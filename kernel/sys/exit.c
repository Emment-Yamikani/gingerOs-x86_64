#include <lib/printk.h>
#include <sys/syscall.h>
#include <arch/x86_64/context.h>

void exit(int exit_code) {
    (void) exit_code;

    panic("%s:%ld: %s(%d);", __FILE__, __LINE__, __func__, exit_code);
}