#include <bits/errno.h>
#include <lib/string.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <sys/_signal.h>

int thread_sigqueue(thread_t *thread, siginfo_t *info) {
    int     err = 0;

    if (!thread || SIGBAD(info->si_signo))
        return -EINVAL;

    thread_assert_locked(thread);

    if (thread_iszombie(thread) ||
        thread_isterminated(thread))
        return -EINVAL;

    if (thread_isstopped(thread) && (
        (info->si_signo != SIGKILL) &&
        (info->si_signo != SIGCONT)))
        return -EINVAL;

    switch ((err = sigismember(&thread->t_sigmask, info->si_signo))) {
    case -EINVAL:
        break;
    case 0:
        queue_lock(&thread->t_sigqueue[info->si_signo - 1]);
        enqueue(&thread->t_sigqueue[info->si_signo - 1], info, 1, NULL);
        queue_unlock(&thread->t_sigqueue[info->si_signo - 1]);
    
        if (err != 0)
            return err;

        if (current != thread)
            err = thread_wake(thread);
        break;
    default:
        return 0;
    }

    return err;
}

int thread_sigdequeue(thread_t *thread, siginfo_t **ret) {
    int         signo = 0;
    siginfo_t   *info = NULL;

    if (!thread || !ret)
        return -EINVAL;

    thread_assert_locked(thread);
    for ( ; signo < NSIG; ++signo) {
        queue_lock(&thread->t_sigqueue[signo]);
        if (queue_count(&thread->t_sigqueue[signo])) {
            if ((sigismember(&thread->t_sigmask, signo + 1)) == 1) {
                queue_unlock(&thread->t_sigqueue[signo]);
                return -EINVAL;
            }
            sigdequeue_pending(&thread->t_sigqueue[signo], &info);
            queue_unlock(&thread->t_sigqueue[signo]);
            sigaddset(&thread->t_sigmask, signo + 1);
            *ret = info;
            return 0;
        }
        queue_unlock(&thread->t_sigqueue[signo]);
    }
    return -ENOENT;
}

int thread_sigmask(thread_t *thread, int how, const sigset_t *restrict set, sigset_t *restrict oset) {
    int err = 0;
    thread_assert_locked(thread);
    
    if (oset)
        *oset = thread->t_sigmask;
    
    if (set == NULL)
        return 0;
    
    if (sigismember(set, SIGKILL) || sigismember(set, SIGSTOP))
        return -EINVAL;

    switch (how) {
    case SIG_BLOCK:
        thread->t_sigmask |= *set;
        break;
    case SIG_UNBLOCK:
        thread->t_sigmask &= ~*set;
        break;
    case SIG_SETMASK:
        thread->t_sigmask = *set;
        break;
    default:
        err = -EINVAL;
        break;
    }
    return err;
}