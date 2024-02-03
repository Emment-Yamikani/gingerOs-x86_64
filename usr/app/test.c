#include <api.h>

void fun(void) {
    loop()
        sys_thread_yield();
}

int main(int argc, char const*argv[]) {
    (void)argc, (void)argv;
    pid_t pid = sys_fork();

    if (pid == 0) {
        sys_setsid();
        pid = sys_fork();
        if (pid) {
            sys_setpgid(pid, sys_getppid());
        }
    }
    
    printf(
        "pid: %d, pgid: %d, sid: %d\n",
        sys_getpid(),
        sys_getpgrp(),
        sys_getsid(sys_getpid())
    );

    loop() sys_thread_yield();
    return 0xDEADBEEF;
}