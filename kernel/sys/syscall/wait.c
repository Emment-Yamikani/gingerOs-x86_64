#include <bits/errno.h>
#include <sys/proc.h>
#include <sys/sysproc.h>
#include <sys/thread.h>
#include <sys/_wait.h>

__unused
pid_t waitpid(pid_t __pid, int *__stat_loc, int __opt) {
    int             err     = 0;            // error code if an error  occurs.
    child_desc_t    desc    = {0};
    (void)__stat_loc, (void)__opt;

    loop() {
        if (__pid < -1) {
            /**
             * get status info of any child process
             * whose process group ID matches pgid.
            */

            desc.child  = NULL;
            desc.flags  = 0;
            desc.pid    = ABS(__pid);
        } else if (__pid == -1) {
            /**
             * Get status info of any child process.
            */
            desc.pid    = 0;
            desc.child  = NULL;
            desc.flags  = 0;
        } else if (__pid == 0) {
            /**
             * Get status info of any child
             * whose process group ID matches
             * that of the calling process.
            */

            desc.child = NULL;
            desc.pid   = getpgrp();
            desc.flags = 0;
        } else if (__pid > 0) {
            /**
             * Get status info of a child process
             * whose process ID matches pid.
            */
            desc.flags  = 0;
            desc.child  = NULL;
            desc.pid    = __pid;
        }

        proc_lock(curproc);
        if ((err = proc_get_child(curproc, &desc))) {
            proc_unlock(curproc);
            return err;
        }

        proc_unlock(curproc);
    }

    return 0;
}