#include <ginger/unistd.h>

void main(void) {
    int     err     = 0;
    // int     fd      = 0;
    mode_t  mode    = 0777;
    devid_t dev     = mkdev(5, 3);
    
    if ((err = mknod("/dev/console", mode, dev)))
        panic("Failed to creat device node: err: %d", err);

    loop();
}