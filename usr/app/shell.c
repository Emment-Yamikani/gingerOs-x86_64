#include <ginger/unistd.h>

void main(void) {
    int         err         = 0;
    utsname_t   name        = {0};
    char        dir[1024]   = {0};

    if ((err = uname(&name)))
        panic("uname failed, err: %d\n", err);

    if ((err = getcwd(dir, sizeof dir)))
        panic("Failed to get current directory, error: %d\n", err);

    printf("[%s@%s %s]$",
        "root",
        name.sysname,
        dir
    );

    loop();
}