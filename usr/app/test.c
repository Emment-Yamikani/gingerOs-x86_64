#include <ginger/unistd.h>

void main(void) {
    char *const argv[] = {"/ramfs/shell", NULL};
    pid_t pid = 0;
    
    printf("Greetings from test...\n");
    
    if ((pid = fork()) == 0) {
        printf("Child process ID: %d\n", getpid());
        execve("/ramfs/shell", argv, NULL);
    }

    printf("PID: %d\n", getpid());
    loop();
}