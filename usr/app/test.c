#include <api.h>
#include <ginger/unistd.h>

void *sys_sbrk(intptr_t);

void main(void) {
    int     err  = 0;
    int     tty  = 0;
    mode_t  mode =  S_IFCHR | S_IRUSR | S_IWUSR |
                    S_IRGRP | S_IWGRP | S_IROTH;
    dev_t   dev  = mkdev(4, 0);

    printf("\n%s is now running...\n", __FILE__);

    if ((err = chdir("/dev/")))
        panic("Failed to change directory. err: %d\n", err);

    if ((err = mknod("tty0", mode, dev)))
        panic("Failed to make device node. err= %d\n", err);

    if ((err = tty = open("tty0", O_RDWR, 0)) < 0)
        panic("Failed to open tty0, err: %d\n", err);

    char *buf = sys_sbrk(100);
    for (int i = 6; i ; --i)
        printf("brk: %p\n", sys_sbrk(100));
    read(tty, buf, sizeof buf);
    write(tty, buf, sizeof buf);

    printf("Reached end of %s:%s()\n", __FILE__, __func__);
    loop();
}