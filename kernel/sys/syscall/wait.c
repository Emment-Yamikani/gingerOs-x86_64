#include <bits/errno.h>
#include <sys/proc.h>
#include <sys/sysproc.h>
#include <sys/thread.h>
#include <sys/_wait.h>
#include <sys/sleep.h>

int proc_get(proc_t *parent, proc_desc_t *desc);

pid_t waitpid(pid_t pid, int *stat_loc, int opt) {
    int     err     = 0;
    int     has_kids= 0;
    __unused int     status  = 0;
    proc_t  *target = NULL;

    if (opt & ~(WNOHANG | WUNTRACED/*WSTOPPED*/ | WEXITED | WCONTINUED | WNOWAIT))
        return -EINVAL;

    loop() {
        has_kids    = 0;
        target      = NULL;

        if (pid < -1) {

        } else if (pid == -1) {
            queue_lock(&curproc->children);
            queue_foreach(proc_t *, proc, &curproc->children) {
            printk("parent process\n");
                has_kids = 1;
                proc_lock(proc);
                debugloc();
                if (proc_isrunning(proc)) {
                    proc_unlock(proc);
                    continue;
                }
                else if (proc_isembryo(proc)) {
                    proc_unlock(proc);
                    continue;
                }
                else {
                    // get status info of dead process.
                    if (opt & WEXITED) {
                        if (proc_iszombie(proc)) {
                            pid = proc->pid; //get pid of child process.
                            if (stat_loc)
                                *stat_loc = proc->exit_code;
                            proc_unlock(proc);
                            return pid;
                        }
                    }

                    // get status info of stopped child.
                    if (opt & WUNTRACED) {
                        // has child process stopped?
                        if (proc_isstopped(proc)) {
                            pid = proc->pid; // get pid of child process.
                            if (stat_loc) // store stat_val in location pointed to by stat_loc.
                                *stat_loc = proc->exit_code;
                            proc_unlock(proc);
                            return pid;
                        }
                    }

                    // get status info of child procss if it has continued execution from stop.
                    if (opt & WCONTINUED) {
                        // has child process continued execution?
                        if (proc_iscontinued(proc)) {
                            pid = proc->pid; // get pid of child process.
                            if (stat_loc) // store stat_val in location pointed to by stat_loc.
                                *stat_loc = proc->exit_code;
                            proc_unlock(proc); // TODO: retmove this after proc_reap() impl.
                            // proc_reap(target);
                            return pid;
                        }
                    }

                    if (opt == 0) {
                        // child changed state.
                        pid = proc->pid; // get pid of child process.
                        if (stat_loc)      // store stat_val in location pointed to by stat_loc.
                            *stat_loc = proc->exit_code;
                        proc_unlock(proc);
                        return pid;
                    }
                }
                proc_unlock(proc);
            }
            queue_unlock(&curproc->children);

            if (has_kids == 0)
                return -ECHILD;

            if (opt & WNOHANG)
                return 0;

            // sleep on child event conditional variable.
            if ((err = cond_wait(&curproc->child_event)))
                break; // break and return error code if was interrupted.
        } else if (pid == 0) {

        } else { // get child status information.
            queue_lock(&curproc->children);
            queue_foreach(proc_t *, proc, &curproc->children) {
                proc_lock(proc);
                if (proc->pid == pid) {
                    target = proc_getref(proc);
                    break;
                }
                proc_unlock(proc);
            }
            queue_unlock(&curproc->children);

            if (target == NULL)
                return -ECHILD;
            
            if (target->parent != curproc) {
                proc_release(target);
                return -EPERM;
            }
            
            // child process still running.
            if (proc_isrunning(target)) {
                // we don't want to wait for this process to change state.
                if (opt & WNOHANG) {
                    proc_release(target);
                    return 0;
                }
            } else if (proc_isembryo(target)) {
                proc_release(target);
                // sleep on child event conditional variable.
                if ((err = cond_wait(&curproc->child_event)))
                    break; // break and return error code if was interrupted.
                continue;
            }

            // get status info of dead process.
            if (opt & WEXITED) {
                if (proc_iszombie(target)) {
                    pid = target->pid; //get pid of child process.
                    if (stat_loc)
                        *stat_loc = target->exit_code;
                    proc_release(target);
                    return pid;
                }
            }    

            // get status info of stopped child.
            if (opt & WUNTRACED) {
                // has child process stopped?
                if (proc_isstopped(target)) {
                    pid = target->pid; // get pid of child process.
                    if (stat_loc) // store stat_val in location pointed to by stat_loc.
                        *stat_loc = target->exit_code;
                    proc_release(target);
                    return pid;
                }
            }

            // get status info of child procss if it has continued execution from stop.
            if (opt & WCONTINUED) {
                // has child process continued execution?
                if (proc_iscontinued(target)) {
                    pid = target->pid; // get pid of child process.
                    if (stat_loc) // store stat_val in location pointed to by stat_loc.
                        *stat_loc = target->exit_code;
                    proc_release(target); // TODO: retmove this after proc_reap() impl.
                    // proc_reap(target);
                    return pid;
                }
            }

            if (opt == 0) {
                // child changed state.
                pid = target->pid; // get pid of child process.
                if (stat_loc)      // store stat_val in location pointed to by stat_loc.
                    *stat_loc = target->exit_code;
                proc_release(target);
                return pid;
            }

            proc_release(target);
            // sleep on child event conditional variable.
            if ((err = cond_wait(&curproc->child_event)))
                break; // break and return error code if was interrupted.
        }
    }

    return err;
}

pid_t wait(int *stat_loc) {
    return waitpid(-1, stat_loc, 0);
}