#include <sys/_signal.h>
#include <lib/stddef.h>
#include <sys/thread.h>


const char *signal_str[] = {
    [SIGABRT - 1] = "SIGABRT",
    [SIGALRM - 1] = "SIGALRM",
    [SIGBUS - 1] = "SIGBUS",
    [SIGCANCEL - 1] = "SIGCANCEL",
    [SIGCHLD - 1] = "SIGCHLD",
    [SIGCONT - 1] = "SIGCONT",
    [SIGEMT - 1] = "SIGEMT",
    [SIGFPE - 1] = "SIGFPE",
    [SIGHUP - 1] = "SIGHUP",
    [SIGILL - 1] = "SIGILL",
    [SIGINT - 1] = "SIGINT",
    [SIGIO - 1] = "SIGIO",
    [SIGIOT - 1] = "SIGIOT",
    [SIGKILL - 1] = "SIGKILL",
    [SIGPIPE - 1] = "SIGPIPE",
    [SIGPROF - 1] = "SIGPROF",
    [SIGQUIT - 1] = "SIGQUIT",
    [SIGSEGV - 1] = "SIGSEGV",
    [SIGSTOP - 1] = "SIGSTOP",
    [SIGSYS - 1] = "SIGSYS",
    [SIGTERM - 1] = "SIGTERM",
    [SIGTRAP - 1] = "SIGTRAP",
    [SIGTSTP - 1] = "SIGTSTP",
    [SIGTTIN - 1] = "SIGTTIN",
    [SIGTTOU - 1] = "SIGTTOU",
    [SIGURG - 1] = "SIGURG",
    [SIGUSR1 - 1] = "SIGUSR1",
    [SIGUSR2 - 1] = "SIGUSR2",
    [SIGVTALRM - 1] = "SIGVTALRM",
    [SIGWINCH - 1] = "SIGWINCH",
    [SIGXCPU - 1] = "SIGXCPU",
    [SIGXFSZ - 1] = "SIGXFSZ",
};

int pause(void);
int sigpending(sigset_t *set);
int kill(pid_t pid, int signo);
unsigned long alarm(unsigned long sec);
sigfunc_t signal(int signo, sigfunc_t func);
int sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);
int sigaction(int signo, const sigaction_t *restrict act, sigaction_t *restrict oact);

int pthread_kill(tid_t tid, int signo) {
    int err = 0;
    thread_t *thread = NULL;

    current_tgroup_lock();
    if ((err = tgroup_get_thread(current_tgroup(), tid, 0, &thread))) {
        current_tgroup_unlock();
        return err;
    }
    current_tgroup_unlock();

    if (signo == 0) {
        thread_unlock(thread);
        return 0;
    }

    if ((err = thread_sigqueue(thread, signo))) {
        thread_unlock(thread);
        goto error;
    }

    thread_unlock(thread);
    return 0;
error:
    return err;
}

int sigwait(const sigset_t *restrict set, int *restrict signop);
int pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);