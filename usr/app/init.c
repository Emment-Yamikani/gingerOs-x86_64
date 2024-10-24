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