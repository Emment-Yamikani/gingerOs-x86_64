#include <ginger/unistd.h>

int fork1(void) {
    pid_t   pid     = 0;
    int     staloc  = 0;

    if ((pid = fork())) {
        return waitpid(pid, &staloc, 0);
    } else if (pid < 0) {
        panic("Failed to fork() a child process\n");
    }
    return 0;
}


void main(void) {
    int     err = 0;

    loop () {
        if (fork1() == 0)
            break;
    }

    if ((err = execve("/ramfs/test", NULL, NULL)))
        panic("Failed to exec() = %d\n", err);
}