#include <ginger/unistd.h>

void main(void) {
    struct utsname name;

    uname(&name);

    printf("[%s-%s %s]$ ",
        name.sysname,
        name.nodename,
        name.sysname
    );

    loop();
}