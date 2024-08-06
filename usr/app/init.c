#include <ginger/unistd.h>
#include <api.h>

void main(void) {
    int     err     = 0;
    int     fd      = 0;
    mode_t  mode    = 0777 | S_IFCHR;

    if ((err = mknod("/dev/null", mode, mkdev(1, 8))))
        panic("Failed to creat device node: err: %d", err);

    if ((err = fd = open("/dev/null", O_RDWR, mode)) < 0)
        panic("Failed to open: device, error: %d\n", err);

    if ((err = read(fd, &mode, 1)) < 0)
        panic("Failed to read from null, err: %d\n", err);

    printf("read from nulldev\n");
    loop() {
    }
}