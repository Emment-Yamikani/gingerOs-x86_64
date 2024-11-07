#include <api.h>
#include <ginger/unistd.h>

void main(void) {
    int     err  = 0;
    int     pts  = 0;
    int     ptmx = 0;
    mode_t  mode =  S_IFCHR | S_IRUSR | S_IWUSR |
                    S_IRGRP | S_IWGRP | S_IROTH;
    dev_t   dev  = mkdev(5, 2);

    if ((err = mknod("/dev/ptmx", mode, dev)))
        panic("Failed to make device node. err= %d\n", err);

    if ((err = ptmx = open("/dev/ptmx", O_RDWR, 0)) < 0)
        panic("Failed to open ptmx,, err: %d\n", err);

    if ((err = pts = open("/dev/pts/0", O_RDWR, 0)) < 0)
        panic("Failed to open pts,, err: %d\n", err);

    printf("Sucessfully opened device\n");

    write(ptmx, "Hello world. :)\n", 17);

    char buf[100] = {0};
    read(pts, buf, sizeof buf);

    printf("pts data: %s", buf);
    panic("Sucessfully opened device\n");
}