#include <sys/proc.h>


pid_t getpid(void) {
    proc_lock(curproc);

    pid_t pid = curproc->pid;

    proc_unlock(curproc);

    return pid;
}

pid_t getppid(void) {
    proc_lock(curproc);

    proc_lock(curproc->parent);
    pid_t pid = curproc->parent->pid;
    proc_unlock(curproc->parent);

    proc_unlock(curproc);

    return pid;
}