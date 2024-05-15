#include <ginger/unistd.h>

void main(void) {
    char *const argv[] = {"/ramfs/shell", NULL};
    pid_t pid = 0;
    
    printf("Greetings...\n");

    
    if ((pid = fork()) == 0) {
        execve("/ramfs/shell", argv, NULL);
    }
    loop();
}