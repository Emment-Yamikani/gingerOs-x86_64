#include <sys/thread.h>
#include <sys/_signal.h>
#include <sync/cond.h>

cond_t *condvar = COND_NEW();

void sa_sigaction(int signo, siginfo_t *info, void *ctx) {
    printk("signo: %d, info: %p, context: %p\n", signo, info, ctx);
}

void test_signal(void) {
    int         err = 0;
    sigaction_t act = {0};

    act.sa_handler  = NULL;
    act.sa_flags    = SA_SIGINFO;
    act.sa_sigaction= sa_sigaction;
    
    err = sigaction(SIGINT, &act, NULL);
    assert(err == 0, "Failed to set signal action!!!");

    cond_signal(condvar);
    loop();
}

void test_main(void) {
    tid_t       tid     = 0;
    int         err     = 0;
    thread_t    *thread = NULL;

    err = kthread_create(
        NULL,
        (void *)test_signal,
        condvar,
        THREAD_CREATE_GROUP |
        THREAD_CREATE_SCHED,
        &thread
    );

    assert(err == 0, "Failed to create new thread!");

    tid = thread_gettid(thread);
    thread_unlock(thread);

    cond_wait(condvar);
    err = pthread_kill(tid, SIGINT);

    assert_msg(err == 0,
        "Failed to send signal to thread[%d], errno: %d\n", tid, err);
} //BUILTIN_THREAD(test_main, test_main, NULL);