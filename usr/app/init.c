#include <ginger/unistd.h>

void handler(int signo) {
    printf("thread(%d:%d) caught signal(%d)\n", gettid(), getpid(), signo);
}

void sig_int(int signo) {
    printf("thread(%d:%d) caught signal(%d)\n", gettid(), getpid(), signo);
}

unsigned pseudo_sleep(unsigned sec) {
    alarm(sec);
    pause();
    return 0;
}

void main(void) {
    signal(SIGALRM, handler);
    pseudo_sleep(2);
    pid_t pid = fork();


    if (pid) {
        alarm(2);
        pause();
        kill(pid, SIGINT);
    } else if (pid == 0) {
        signal(SIGINT, sig_int);
        pause();
    } else {
        panic("error: fork()\n");
    }

    loop();
}