#include <sys/thread.h>
#include <lib/printk.h>
#include <sys/_signal.h>
#include <ginger/jiffies.h>

void signal_handler(int signo) {
    printk("signo %d occured\n", signo);
    jiffies_timed_wait(3);
}

void sa_sigaction(int signo, siginfo_t *info, void *context) {
    printk(
        "signo %d\n"
        "info: %p\n"
        "context: %p\n",
        signo,
        info,
        context
    );
    loop();
} 

void test (cond_t *cv) {
    int err = 0;
    sigset_t set;
    sigaction_t act = {0};

    sigemptyset(&set);

    BUILTIN_THREAD_ANOUNCE("Good");
    act.sa_flags        = 0;
    act.sa_mask         = set;
    act.sa_handler      = (sigfunc_t)signal_handler;

    assert_msg(!(err = sigaction(SIGINT, &act, NULL)),
        "sigaction failed, err = %d", err);

    act.sa_handler      = NULL;
    act.sa_flags        = SA_SIGINFO;
    act.sa_sigaction    = sa_sigaction;
    assert_msg(!(err = sigaction(SIGUSR1, &act, NULL)),
        "sigaction failed, err = %d", err);

    cond_signal(cv);

    loop() thread_yield();
}

COND_VAR(cv);

void test_main(void) {
    tid_t       tid     = 0;
    thread_t    *thread = NULL;

    BUILTIN_THREAD_ANOUNCE("Hello");

    kthread_create(NULL, (void *)test,
    cv, THREAD_CREATE_SCHED, &thread);

    tid = thread->t_tid;
    thread_unlock(thread);

    cond_wait(cv);

    pthread_kill(tid, SIGINT);
    pthread_kill(tid, SIGUSR1);
    pthread_kill(tid, SIGINT);
    pthread_kill(tid, SIGINT);
    loop() thread_yield();
}

BUILTIN_THREAD(test_main, test_main, NULL);