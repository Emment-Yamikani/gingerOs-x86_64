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
    uid_t uid = 0;

    proc_assert_locked(proc);
    proc_assert_locked(curproc);

    if (proc != curproc) {
        cred_lock(curproc->cred);
        if ((curproc->pid == 0) ||
            ((uid = curproc->cred->c_uid) == 0) ||
            (curproc->cred->c_euid == 0) ||
            (curproc->cred->c_gid == 0) ||
            (curproc->cred->c_egid == 0)) {
            cred_unlock(curproc->cred);
            if (ppuid)
                *ppuid = uid;
            return 0;
        }

        cred_lock(proc->cred);
        if ((proc->cred->c_uid != (uid = curproc->cred->c_uid)) &&
            (proc->cred->c_euid != curproc->cred->c_euid) &&
            (proc->cred->c_gid != curproc->cred->c_gid) &&
            (proc->cred->c_egid != curproc->cred->c_egid)) {
            cred_unlock(proc->cred);
            cred_unlock(curproc->cred);
            return -EPERM;
        }

        cred_unlock(proc->cred);
        cred_unlock(curproc->cred);
    }

    if (ppuid)
        *ppuid = uid;
    return 0;
}

int signal_send(proc_t *proc, int signo) {
    int         err         = 0;
    uid_t       uid         = 0;
    siginfo_t   *info       = NULL;
    sig_desc_t  *sigdesc    = NULL;

    if (signo < 0 || signo > NSIG)
        return -EINVAL;

    proc_assert_locked(proc);
    proc_assert_locked(curproc);

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
    info->si_pid    = curproc->pid;
    info->si_value  = (union sigval) {
        .sigval_int = signo
    };

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


    if (pid > 0) { // send to process whose process ID == pid.
        if (pid == getpid()) { // self signal was implied.
            proc_lock(curproc);
            proc = curproc; // get a reference to self
        } else if ((err = procQ_search_bypid(pid, &proc)))
            return err;

        // Not sending to self so lock current process struct.
        if (proc != curproc)
            proc_lock(curproc);
        
        // send the signal to the process in question
        if ((err = signal_send(proc, signo))) {
            // FIXME: consider divating to an error: lable.
            if (proc != curproc)
                proc_unlock(curproc);
            proc_unlock(proc);
            return err;
        }

        if (proc != curproc)
            proc_unlock(curproc);
        
        proc_unlock(proc);

    } else if (pid == 0) { // send to all processes whose pgid == that of sending process' pgid
        proc_lock(curproc);
        // foreach proc, send a signal if it's pgid match sending proc's pgid
        queue_foreach(proc_t *, proc, procQ) {
            // skip the current proc
            if (proc == curproc)
                continue;
            
            proc_lock(proc);
            if (proc->pgid == curproc->pgid) {
                if ((err = signal_send(proc, signo))) {
                    // FIXME: consider divating to an error: lable.
                    proc_unlock(proc);
                    proc_unlock(curproc);
                    return err;
                }
            }
            proc_unlock(proc);
        }
        proc_unlock(curproc);
    } else if (pid == -1) { // send to all process, except an unspecified set of sysprocs.
        proc_lock(curproc);
        queue_foreach(proc_t *, proc, procQ) {
            if (proc == curproc) // skip current proc.
                continue;
            
            proc_lock(proc);
            if ((err = signal_send(proc, signo))) {
                // FIXME: consider divating to an error: lable.
                proc_unlock(proc);
                proc_unlock(curproc);
                return err;
            }
            proc_unlock(proc);
        }
        proc_unlock(curproc);
    } else { // PID < -1, send to all processes whose PGID == ABS(pid).
        proc_lock(curproc);
        // foreach proc, send a signal if it's pgid match sending proc's pgid
        queue_foreach(proc_t *, proc, procQ) {
            // skip the current proc
            if (proc == curproc)
                continue;
            
            proc_lock(proc);
            if (proc->pgid == ABSi(pid)) {
                if ((err = signal_send(proc, signo))) {
                    // FIXME: consider divating to an error: lable.
                    proc_unlock(proc);
                    proc_unlock(curproc);
                    return err;
                }
            }
            proc_unlock(proc);
        }
        proc_unlock(curproc);
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
