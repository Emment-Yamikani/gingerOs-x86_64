#include <ginger/unistd.h>

void handler(int);

void main(void) {
    pid_t pid = fork();

    if (pid != 0) {
        kill(pid, SIGINT);
    } else if (pid == 0) {
        loop();
    }

    printf("Okay done sending signal\n");
    loop();
}


void handler(int signo) {
    printf("%s:%d: caught signal(%d)\n",
    __FILE__, __LINE__, signo);
}