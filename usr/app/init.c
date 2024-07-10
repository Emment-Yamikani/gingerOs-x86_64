#include <ginger/unistd.h>

void handler(int);

void main(void) {
 
    pid_t pid = fork();
    signal(SIGINT, handler);

    if (pid) {
        sleep(2);
        kill(pid, SIGINT);
    } else {
    }

    loop();
}


void handler(int signo) {
    printf("%s:%d: caught signal(%d)\n", __FILE__, __LINE__, signo);
}