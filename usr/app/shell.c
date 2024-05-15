#include <ginger/unistd.h>

void main(void) {
    utsname_t name;
    char dir[1024] = {0};

    uname(&name);
    getcwd(dir, sizeof dir);

    printf("[%s@%s %s]$",
        "root",
        name.sysname,
        dir
    );

    loop();
}