#include <api.h>

void signal_handler(int signo) {
    printf(
        "[pid: %d, tid: %d]"
        " signo: %d\n",
        sys_getpid(),
        sys_thread_self(),
        signo
    );
    // loop();

}

void sa_sigaction(int signo, siginfo_t *info, void *context) {
    printf("signo %d, info: %p, ctx:  %p, pid:  %d\n",
        signo, info, context, info->si_pid
    );
    // loop();
} 

void *test(tid_t tid) {
    
    int         err = 0;
    sigset_t    set = 0;
    sigaction_t act = {0};

    sigemptyset(&set);

    act.sa_flags        = 0;
    act.sa_mask         = set;
    act.sa_handler      = (sigfunc_t)signal_handler;

    assert_msg(!(err = sys_sigaction(SIGINT, &act, NULL)),
        "sigaction failed, err = %d", err);

    act.sa_handler      = NULL;
    act.sa_flags        = SA_SIGINFO;
    act.sa_sigaction    = sa_sigaction;
    assert_msg(!(err = sys_sigaction(SIGUSR1, &act, NULL)),
        "sigaction failed, err = %d", err);

    assert_msg(!(err = sys_sigaction(SIGUSR2, &act, NULL)),
               "sigaction failed, err = %d", err);

    assert_msg(!(err = sys_sigaction(SIGTRAP, &act, NULL)),
               "sigaction failed, err = %d", err);
    
    sys_unpark(tid);
    loop();
}

void main (void) {
    tid_t   tid = 0;

    sys_thread_create(
        &tid, NULL, (void *)test,
        (void *)((long)sys_thread_self())
    );

    sys_park();

    sys_pthread_kill(tid, SIGTRAP);
    sys_pthread_kill(tid, SIGUSR1);
    sys_pthread_kill(tid, SIGINT);
    sys_pthread_kill(tid, SIGUSR2);
    sys_pthread_kill(tid, SIGINT);

    printf("At the end of main();\n");
    loop();
}