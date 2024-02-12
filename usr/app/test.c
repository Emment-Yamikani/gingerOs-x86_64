#include <api.h>

void fun(void) {
    loop() sys_thread_yield();
}

int main(int argc, char const*argv[]) {
    (void)argc, (void)argv;
    pid_t pid = sys_fork();


    if (pid < 0)
        sys_exit(pid);
    else if (pid == 0) {
        sys_exit(0);
    }

    if (sys_getpid() == 1)
        loop() sys_thread_yield();
    return 0xDEADBEEF;
}