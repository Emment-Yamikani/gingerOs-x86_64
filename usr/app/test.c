#include <api.h>

void fun(void) {
    printf("[%d:%d]: %s();\n", sys_thread_self(), sys_getpid(), __func__);
    loop() sys_thread_yield();
}

pid_t fork(void) {
    return sys_fork();
}

int waitpid(pid_t pid, int *stat_loc, int opt) {
    return sys_waitpid(pid, stat_loc, opt);
}

int main(void) {
    pid_t   pid         = 0;
    int     statloc     = 0;

    if ((pid = fork()) < 0) {
        panic("Failed to fork child process. error: %d\n", pid);
    } else if (pid != 0) { // init process.
        loop() {
            waitpid(-1, &statloc, 0);
        }
    }

    // child process.
    pid = sys_setpgrp();

    if (0 > (pid = sys_fork())) // error occured.
        panic("Failed to fork(), sys_fork() returned =%d\n", pid);
    else if (pid == 0) { // child process.
        printf("PID: %d, PGID: %d, SID: %d\n", sys_getpid(), sys_getpgrp(), sys_getsid(0));
        sys_sleep(1); // Child thread simulates some work.
        sys_exit(1); // then exists.
    } else { // parent process.
        pid = sys_waitpid(0, &statloc, WEXITED); // wait for child process to exit.
        printf("Child: %d, changed state, stat_val= %d\n", pid, statloc);
        loop();
    }
    return 0xDEADBEEF;
}