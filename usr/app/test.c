#include <api.h>

void fun(void) {
    printf("[%d:%d]: %s();\n", sys_thread_self(), sys_getpid(), __func__);
    loop() sys_thread_yield();
}

int main(void) {
    pid_t   pid         = 0;
    int     statloc     = 0;

    if ((pid = sys_fork()) < 0)
        panic("[%d:%d]: Failed to fork a child.", sys_thread_self(), sys_getpid());
    else if (pid)
        sys_waitpid(pid, &statloc, 0);
    else {
        if ((sys_thread_create(NULL, NULL, (thread_entry_t)fun, NULL))) {
            panic("[%d:%d]: Failed to create new thread in process.", sys_thread_self(), sys_getpid());
        }

        sys_exit(0);
    }

    fun();
    return 0xDEADBEEF;
}