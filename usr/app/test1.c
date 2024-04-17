#include <api.h>

void signal_handler(int signo) {
    printf(
        "[pid: %d, tid: %d]"
        " signo: %d\n",
        sys_getpid(),
        sys_thread_self(),
        signo
    );
    loop();

}

void sa_sigaction(int signo, siginfo_t *info, void *context) {
    printf(
        "signo %d\n"
        "info: %p\n"
        "context: %p\n",
        signo,
        info,
        context
    );

    printf(
        "signaling pid: %d\n",
        info->si_pid
    );
    loop();
} 

void *test(void *arg) {
    (void)arg;
    
    int err = 0;
    sigset_t set;
    sigaction_t act = {0};

    sigemptyset(&set);

    spin_lock((spinlock_t *)arg);

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
    
    spin_unlock((spinlock_t *)arg);

    sys_sleep(1);
    loop();
}


    spinlock_t *lk = SPINLOCK_NEW();
void main (void) {
    tid_t tid = 0;

    sys_thread_create(&tid, NULL, test, lk);
    sys_sleep(2);
    spin_lock((spinlock_t *)lk);

    sys_pthread_kill(tid, SIGINT);
    sys_pthread_kill(tid, SIGUSR1);
    sys_pthread_kill(tid, SIGINT);
    sys_pthread_kill(tid, SIGINT);
    loop();
}