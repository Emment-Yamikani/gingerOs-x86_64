#include <ginger/unistd.h>
#include <api.h>

void main(void) {
    pid_t   sh     = 0;
    int     staloc = 0;
    char *const envp[] = {
        "SHELL=/shell",
        "INIT=/ramfs/init",
        NULL
    };
    char *const argp[] = {
        "/ramfs/shell",
        NULL
    };

    int err = 0;
    int fd  = 0;
    struct stat st;
    char *buf = NULL;

    if ((err = stat("/ramfs/dev", &st)))
        panic("error: %d.", err);

    if (NULL == (buf = malloc(st.st_size + 1)))
        panic("Failed to allocate memory.");

    memset(buf, 0, st.st_size);

    if ((err = fd = open("/ramfs/dev", O_RDONLY, 0)))
        panic("Failed to read device lookup list. err: %d", err);

    read(fd, buf, st.st_size);
    
    loop() {
        if ((sh = fork()) < 0)
            panic("Failed to fork shell\n");
        else if (sh != 0) {
            sh = wait(&staloc);
            if (staloc) panic("%d: Ended with error: %d\n", sh, staloc);
        } else {
            if ((execve(*argp, argp, envp)))
                panic("Failed to exec shell\n");
        }
    }
}