#include <arch/cpu.h>
#include <bits/errno.h>
#include <sys/_signal.h>
#include <mm/kalloc.h>
#include <sys/sysproc.h>
#include <arch/thread.h>
#include <arch/signal.h>
#include <sys/proc.h>
#include <sys/thread.h>

int signal_perm(proc_t *proc, uid_t *ppuid) {
    uid_t uid   = 0;
    uid_t gid   = 0;
    uid_t euid  = 0;
    uid_t egid  = 0;

    if (proc == NULL)
        goto __not_granted;

    proc_assert_locked(proc);

    if (current == NULL)
        goto __granted;
    
    cred_lock(current->t_cred);
    uid = current->t_cred->c_uid;
    
    if (curproc == proc) {
        cred_unlock(current->t_cred);
        goto __granted;
    }

    gid  = current->t_cred->c_gid;
    euid = current->t_cred->c_euid;
    egid = current->t_cred->c_egid;
    cred_unlock(current->t_cred);

    if (proc != curproc) {
        if (!getpid()   || (uid == 0)  || (euid == 0) ||
            (gid == 0)  || (egid == 0))
            goto __granted;

        cred_lock(proc->cred);
        if ((proc->cred->c_uid  != uid) &&
            (proc->cred->c_euid != euid) &&
            (proc->cred->c_gid  != gid) &&
            (proc->cred->c_egid != egid)) {
            cred_unlock(proc->cred);
            goto __not_granted;
        }
        cred_unlock(proc->cred);
    }

__granted:
    if (ppuid)
        *ppuid = uid;
    return 0;
__not_granted:
    return -EPERM;
}

int signal_send(proc_t *proc, int signo) {
    int         err         = 0;
    uid_t       uid         = 0;
    siginfo_t   *info       = NULL;
    sig_desc_t  *sigdesc    = NULL;

    if (signo < 0 || signo > NSIG)
        return -EINVAL;

    proc_assert_locked(proc);

    if ((err = signal_perm(proc, &uid)))
        return err;
    else if (signo == 0)
        return 0; // Test Ok
    
    if (NULL == (info = (siginfo_t *)kmalloc(sizeof *info)))
        return -ENOMEM;
    
    info->si_code   = 0;
    info->si_status = 0;
    info->si_uid    = uid;
    info->si_addr   = NULL;
    info->si_signo  = signo;
    info->si_value  = (union sigval) { .sigval_int = signo };
    info->si_pid    = proc != curproc ? getpid() : curproc->pid;

    sigdesc = proc->sigdesc;
    sigdesc_lock(sigdesc);
    queue_lock(&sigdesc->sig_queue[signo - 1]);
    if ((err = sigenqueue_pending(
        &sigdesc->sig_queue[signo - 1], info))) {
        queue_unlock(&sigdesc->sig_queue[signo - 1]);
        sigdesc_unlock(sigdesc);
        kfree(info);
        return err;
    }
    queue_unlock(&sigdesc->sig_queue[signo - 1]);
    sigdesc_unlock(sigdesc);
    return 0;
}

int kill(pid_t pid, int signo) {
    int     err     = 0;
    proc_t  *proc   = NULL;
    
    if (pid > 0) {
        if ((err = procQ_search_bypid(pid, &proc)))
            return err;
        
        if ((err = signal_send(proc, signo))) {
            proc_unlock(proc);
            return err;
        }

        proc_unlock(proc);
    } else if (pid == 0) {

    } else if (pid == -1) {

    } else {

    }

    return 0;
}

int pthread_kill(tid_t tid, int signo) {
    int         err     = 0;
    siginfo_t   *info   = NULL;
    thread_t    *thread = NULL;

    if (SIGBAD(signo))
        return -EINVAL;

    if ((err = thread_get(tid, 0, &thread)))
        return err;

    if (signo == 0) {
        thread_unlock(thread);
        return 0;
    }

    if (NULL == (info = (siginfo_t *)kmalloc(sizeof *info))) {
        thread_putref(thread);
        thread_unlock(thread);
        return -ENOMEM;
    }

    memset(info, 0, sizeof *info);

    info->si_addr   = NULL;
    info->si_signo  = signo;
    info->si_pid    = getpid();

    if ((err = thread_sigqueue(thread, info))) {
        thread_putref(thread);
        thread_unlock(thread);
        goto error;
    }

    thread_putref(thread);
    thread_unlock(thread);
    return 0;
error:
    if (info)
        kfree(info);

    return err;
}
