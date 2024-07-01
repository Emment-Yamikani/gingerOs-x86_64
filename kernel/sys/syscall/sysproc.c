#include <sys/proc.h>
#include <sys/sysproc.h>

pid_t getsid(pid_t pid) {
    int     err     = 0;
    pid_t   mysess  = 0;
    proc_t  *proc   = NULL;

    proc_lock(curproc);
    mysess = curproc->sid;
    proc_unlock(curproc);

    if (pid == 0)
        return mysess;

    if ((err = procQ_search_bypid(curproc->pid, &proc)))
        return err;

    if (mysess != proc->sid)
        pid = -EPERM;
    else
        pid = proc->sid;

    proc_release(proc);
    return pid;
}

pid_t   setsid(void) {
    pid_t   pid     = 0;
    
    if (procQ_search_bypgid(curproc->pid, NULL) == 0) {
        proc_unlock(curproc);
        return -EPERM;
    }
    
    proc_lock(curproc);

    if (curproc->pid == curproc->pgid) {
        proc_unlock(curproc);
        return -EPERM;
    }

    curproc->sid        = curproc->pid;
    pid = curproc->pgid   = curproc->pid;
    proc_unlock(curproc);
    return pid;
}

pid_t getpgrp(void) {
    pid_t pid = 0;
    proc_lock(curproc);
    pid = curproc->pgid;
    proc_unlock(curproc);
    return pid;
}

pid_t setpgrp(void) {
    return setpgid(0, getpid()) == 0 ? getpid() : -EPERM;
}

pid_t getpgid(pid_t pid) {
    int     err = 0;
    proc_t  *proc = NULL;

    if (pid < 0)
        return -EINVAL;

    if (pid == 0)
        return getpgrp();
    
    if ((err = procQ_search_bypid(pid, &proc)))
        return err;
    
    if (proc != curproc) {
        proc_lock(curproc);
        if (proc->sid != curproc->sid) {
            proc_unlock(curproc);
            proc_release(proc);
            return -EPERM;
        }
        proc_unlock(curproc);
    }

    pid = proc->pgid;

    proc_release(proc);
    return pid;
}

int   setpgid(pid_t pid, pid_t pgid) {
    int err = 0;
    proc_t *proc = NULL;
    proc_t *leader = NULL;

    /**The value of the pgid argument
     * is less than 0, or is not a value
     * supported by the implementation.*/
    if (pgid < 0)
        return -EINVAL;

    /**As  a special case, if pid is 0,
     * the process ID of the calling process
     * shall be used. Also, if pgid is 0, the
     * process ID of the indicated process shall be used.*/
    if (pid == 0)
        pid = getpid();

    if ((err = procQ_search_bypid(pid, &proc))) {
        /**The value of the pid argument does
         * not match the process ID of the
         * calling process or of a child process
         * of the calling process.*/
        return err;
    }
    
    // proc is child of calling process.
    if (proc->parent == curproc) {
        /**The value of the pid argument matches the process ID
         * of a child process of the calling process and the child
         * process has  successfully 
         * executed one of the exec functions*/
        if (proc_hasexeced(proc)) {
            proc_release(proc);
            return -EACCES;
        }

        proc_lock(curproc);

        /**The  value of the pid argument matches
         * the process ID of a child process
         * of the calling process and the child
         * process is not in the same sid as
         * the calling process.*/
        if (proc->sid != curproc->sid) {
            proc_unlock(curproc);
            proc_release(proc);
            return -EPERM;
        }
        proc_unlock(curproc);
    }

    /**The process indicated by the pid
     * argument is a sid leader.*/
    if (proc->pid == proc->sid) {
        proc_release(proc);
        return -EPERM;
    }
    
    queue_lock(procQ);
    forlinked (node, procQ->head, node->next) {
        leader = node->data;
        if (leader == proc) {
            leader = NULL;
            continue;
        }
        proc_lock(leader);
        if (leader->pgid == pgid) {
            proc_getref(leader);
            break;
        }
        proc_unlock(leader);
        leader = NULL;
    }
    queue_unlock(procQ);

    /**The value of the pgid argument is valid
     * but does not match the process ID of the
     * process indicated by the pid argument and
     * there is  no  process with a process group ID
     * that matches the value of the pgid argument
     * in the same sid as the calling process.*/
    if (proc->pid != pgid && leader == NULL) {
        proc_release(proc);
        return -EPERM;
    }

    if (leader) {
        if (leader != curproc) {
            proc_lock(curproc);
            if (leader->sid != curproc->sid) {
                proc_release(leader);
                proc_unlock(curproc);
                return -EPERM;
            }
            proc_unlock(curproc);
        }
    }
    
    proc->pgid = pgid;

    if (leader)
        proc_release(leader);

    proc_release(proc);
    return 0;
}