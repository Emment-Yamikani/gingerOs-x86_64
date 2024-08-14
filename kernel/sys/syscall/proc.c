#include <sys/proc.h>

pid_t getpid(void) {
    return curproc ? curproc->pid : 0;
}

pid_t getppid(void) {
    pid_t pid = 0;

    proc_lock(curproc);

    if (curproc->parent == NULL) {
        pid = -EINVAL;
        goto out;
    }

    proc_lock(curproc->parent);
    pid = curproc->parent->pid;
    proc_unlock(curproc->parent);

out:
    proc_unlock(curproc);

    return pid;
}