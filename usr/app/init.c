#include <api.h>

void signal_handler(int signo) {
    printf("signo: %d\n", signo);
}

void sa_sigaction(int signo, siginfo_t *info, void *uctx) {
    printf("signo: %d, info: %p, context: %p\n", signo, info, uctx);
}

void test_signal(tid_t tid) {
    int         err = 0;
    sigaction_t act = {0};
    sigaction_t oact = {0};

    sigemptyset(&act.sa_mask);
    sigfillset(&act.sa_mask);
    sigdelset(&act.sa_mask, SIGSTOP);
    sigdelset(&act.sa_mask, SIGKILL);
    sigdelset(&act.sa_mask, SIGINT);

    act.sa_handler  = NULL;
    act.sa_flags    = SA_SIGINFO;
    act.sa_sigaction= sa_sigaction;
    err = sys_sigaction(SIGINT, &act, &oact);

    assert_msg(
        err == 0,
        "%s:%d: Failed to set signal action, error: %d\n",
        __FILE__, __LINE__, err
    );

    sigaddset(&act.sa_mask, SIGINT);
    sigdelset(&act.sa_mask, SIGUSR1);

    act.sa_flags     = 0;
    act.sa_sigaction = NULL;
    act.sa_handler   = signal_handler;
    err = sys_sigaction(SIGUSR1, &act, &oact);

    assert_msg(
        err == 0,
        "%s:%d: Failed to set signal action, error: %d\n",
        __FILE__, __LINE__, err
    );

    sys_unpark(tid);

    loop();
}

void main(void) {
    int     err = 0;
    tid_t   tid = 0;

    err = sys_thread_create(
        &tid,
        NULL,
        (void *)test_signal,
        (void *)(long)sys_thread_self()
    );

    assert_msg(
        err == 0,
        "%s:%d: Failed to create thread, error: %d",
        __FILE__, __LINE__, err
    );

    sys_park();
    sys_pthread_kill(tid, SIGUSR1);
    sys_pthread_kill(tid, SIGINT);
    loop();
}