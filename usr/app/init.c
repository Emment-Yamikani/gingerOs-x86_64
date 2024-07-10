#include <ginger/unistd.h>

void handler(int signo) {
    printf("thread(%d) caught signal: %d\n", gettid(), signo);
}

void t1(void) {
    pause();
    loop();
}

void t2(void) {
    pause();
    loop();
}

void main(void) {
    pid_t pid = fork();
    signal(SIGINT, handler);

    if (pid) {
        sleep(1);
        for (int i = 6; i; sleep(1), --i)
            kill(pid, SIGINT);
        printf("done sending");
    } else if (pid == 0) {
        for (int i = 5; i ; --i) {
            thread_create(NULL,
            NULL,
            (void *)t1,
            NULL
            );
        }

        thread_create(NULL,
            NULL,
            (void *)t2,
            NULL
        );
    }

    loop();
}