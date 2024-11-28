#include <bits/errno.h>
#include <sys/proc.h>
#include <sys/sysproc.h>
#include <sys/thread.h>
#include <sys/_wait.h>
#include <sys/sleep.h>

int proc_get(proc_t *parent, proc_desc_t *desc);

/**
 * @brief Waits for a child process to change state.
 * 
 * @param pid The process ID or process group ID to wait for. Special values:
 *            - < -1: Wait for any child process whose process group ID is equal to the absolute value of pid.
 *            - -1: Wait for any child process.
 *            - 0: Wait for any child process whose process group ID is equal to that of the calling process.
 *            - > 0: Wait for the child whose process ID is equal to pid.
 * @param stat_loc Pointer to an integer to store the status information.
 * @param options Bitwise OR of options (WNOHANG, WUNTRACED, WEXITED, WCONTINUED).
 * 
 * @return The process ID of the child process that changed state, or a negative error code.
 */
pid_t waitpid(pid_t pid, int *stat_loc, int options) {
    int     err      = 0;
    int     has_kids = 0;
    proc_t  *target  = NULL;

    // Check for deadlock if waiting for self.
    if (pid == getpid()) {
        return -EDEADLK;
    }

    // Validate options.
    if (options & ~(WNOHANG | WUNTRACED |
        WEXITED | WCONTINUED | WNOWAIT)) {
        return -EINVAL;
    }

    loop() {
        has_kids = 0;
        target   = NULL;

        // Case 1: Wait for any child process whose process group ID is equal to the absolute value of pid.
        if (pid < -1) {
            queue_lock(procQ);
            queue_foreach(proc_t *, proc, procQ) {
                if (proc == curproc)
                    continue;

                proc_lock(proc);
                if ((proc->pgid == ABSi(pid)) && (proc->parent == curproc)) {
                    has_kids = 1;
                    if (((options & WEXITED)    && proc_iszombie(proc))   ||
                        ((options & WUNTRACED)  && proc_isstopped(proc))  ||
                        ((options & WCONTINUED) && proc_iscontinued(proc))||
                        ((options == 0)         && (proc_iszombie(proc)   ||
                        proc_isstopped(proc)    || proc_iscontinued(proc)))) {
                        pid = proc->pid;
                        if (stat_loc) {
                            *stat_loc = proc->exit_code;
                        }
                        proc_unlock(proc);
                        queue_unlock(procQ);
                        return pid;
                    }
                }
                proc_unlock(proc);
            }
            queue_unlock(procQ);
            goto wait_group;

        // Case 2: Wait for any child process.
        } else if (pid == -1) {
            queue_lock(&curproc->children);
            queue_foreach(proc_t *, proc, &curproc->children) {
                has_kids = 1;
                proc_lock(proc);
                if (((options & WEXITED)    && proc_iszombie(proc))   ||
                    ((options & WUNTRACED)  && proc_isstopped(proc))  ||
                    ((options & WCONTINUED) && proc_iscontinued(proc))||
                    ((options == 0)         && (proc_iszombie(proc)   ||
                    proc_isstopped(proc)    || proc_iscontinued(proc)))) {
                    pid = proc->pid;
                    if (stat_loc) {
                        *stat_loc = proc->exit_code;
                    }
                    proc_unlock(proc);
                    queue_unlock(&curproc->children);
                    return pid;
                }
                proc_unlock(proc);
            }
            queue_unlock(&curproc->children);
            goto wait_group;

        // Case 3: Wait for any child process whose process group ID is equal to that of the calling process.
        } else if (pid == 0) {
            queue_lock(procQ);
            queue_foreach(proc_t *, proc, procQ) {
                if (proc == curproc)
                    continue;

                proc_lock(proc);
                if ((proc->pgid == getpgrp()) && (proc->parent == curproc)) {
                    has_kids = 1;
                    if (((options & WEXITED)    && proc_iszombie(proc))   ||
                        ((options & WUNTRACED)  && proc_isstopped(proc))  ||
                        ((options & WCONTINUED) && proc_iscontinued(proc))||
                        ((options == 0)         && (proc_iszombie(proc)   ||
                        proc_isstopped(proc)    || proc_iscontinued(proc)))) {
                        pid = proc->pid;
                        if (stat_loc) {
                            *stat_loc = proc->exit_code;
                        }
                        proc_unlock(proc);
                        queue_unlock(procQ);
                        return pid;
                    }
                }
                proc_unlock(proc);
            }
            queue_unlock(procQ);
            goto wait_group;

        // Case 4: Wait for the child whose process ID is equal to pid.
        } else {
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

            if (target == NULL) {
                return -ECHILD;
            }

            if (target->parent != curproc) {
                proc_release(target);
                return -EPERM;
            }

            if (((options & WEXITED)    && proc_iszombie(target))   ||
                ((options & WUNTRACED)  && proc_isstopped(target))  ||
                ((options & WCONTINUED) && proc_iscontinued(target))||
                ((options == 0)         && (proc_iszombie(target)   ||
                proc_isstopped(target)  || proc_iscontinued(target)))) {
                pid = target->pid;
                if (stat_loc) {
                    *stat_loc = target->exit_code;
                }
                proc_release(target);
                return pid;
            }

            // If WNOHANG is specified and the child is still running, return immediately.
            if ((options & WNOHANG)) {
                proc_release(target);
                return 0;
            }

            proc_release(target);
            if ((err = cond_wait(&curproc->child_event))) {
                return err;
            }
        }

        continue;
    wait_group:
        if (has_kids == 0) {
            return -ECHILD;
        }

        if (options & WNOHANG) {
            return 0;
        }

        if ((err = cond_wait(&curproc->child_event))) {
            return err;
        }
    }

    return err;
}