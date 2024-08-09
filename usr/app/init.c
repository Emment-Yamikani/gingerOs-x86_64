#include <ginger/unistd.h>
#include <api.h>

void main(void) {
    int         err     = 0;
    pid_t       pid     = 0;
    int         stat_loc= 0;
    int         count   = 0;
    char *const  shell[] = {"/ramfs/shell", NULL};

    loop() {
        if ((err = pid = fork()) < 0) {
            panic("Failed to fork child. err: %d\n", err);
        } else if (pid == 0) {
            if ((err = execve(shell[0], shell, NULL)))
                panic("Failed to execve(), err: %d\n", err);
        }

        if ((err = pid = waitpid(pid, &stat_loc, WNOHANG)) < 0)
            panic("Failed to wait for child, err: %d\n", err);
        printf("init: %d\n", count++);
    }
}