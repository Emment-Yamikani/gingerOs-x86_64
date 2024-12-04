#include <ginger/unistd.h>

void main(void) {
    struct utsname name;
    char *console = "/dev/console";
    dev_t dev = mkdev(5, 1);

    mknod(console, S_IFCHR | 0777, dev);

    uname(&name);

    char buf[1024];
    memset(buf, 0, sizeof buf);

    if (getcwd(buf, sizeof buf))
        panic("Failed to get working directory.\n");    

    printf("[%s-%s %s]$ ",
        name.sysname,
        name.nodename,
        buf
    );

    loop();
}