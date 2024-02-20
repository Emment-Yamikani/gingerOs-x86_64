#include <api.h>

void fun(void) {
    printf("[%d:%d]: %s();\n", sys_thread_self(), sys_getpid(), __func__);
    loop() sys_thread_yield();
}

int main(void) {
    pid_t   pid         = 0;
    int     statloc     = 0;
    
    sys_thread_create(NULL, NULL, (thread_entry_t)fun, NULL);

    if (0 > (pid = sys_fork())) // error occured.
        panic("Failed to fork(), sys_fork() returned =%d\n", pid);
    else if (pid == 0) { // child process.
        sys_sleep(3); // Child thread simulates some work.
        printf("Child dying\n");
        sys_exit(1); // then exists.
    } else { // parent process.
    printf("callinf sys_wait();\n");
        pid = sys_wait(&statloc); // wait for child process to exit.
        printf("Child changed state, stat_val= %d\n", statloc);
        loop();
    }
    return 0xDEADBEEF;
}