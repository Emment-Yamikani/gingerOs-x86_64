#include <ginger/unistd.h>

void main(void) {
    struct utsname name;
    char *console = "/dev/console";
    dev_t dev = mkdev(5, 1);

    mknod(console, S_IFCHR | 0777, dev);

    int f = open(console, O_RDWR, 0);

    printf("f is open at: %d\n", f);

    write(f, "Hello, device", 14);

    uname(&name);



    printf("[%s-%s %s]$ ",
        name.sysname,
        name.nodename,
        name.sysname
    );

    loop();
}