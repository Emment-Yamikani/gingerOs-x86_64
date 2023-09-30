#include <lib/types.h>
#include <sys/thread.h>
#include <sys/sleep.h>

void handler(int signo) {
    printk("%s on thread[%d]\n", signal_str[signo - 1], thread_self());
}

void B(void) {
    loop();
}
BUILTIN_THREAD(B, B, NULL);

void core_start(void) {
    sigset_t set;
    sigaction_t act;
    sigfillset(&set);
    sigdelset(&set, SIGKILL);
    sigdelset(&set, SIGSTOP);
    act.sa_mask = set;
    sigdelset(&set, SIGXFSZ);
    act.sa_flags = 0;
    act.sa_handler = handler;
    act.sa_sigaction = NULL;
    sigaction(SIGXFSZ, &act, NULL);
    printk("thread[%d]: tgroup[%d]:\n", thread_self(), current_tgroup()->tg_tgid);
    loop() {
        jiffies_timed_wait(1);
        printk("%s\n", __func__);
    };
}