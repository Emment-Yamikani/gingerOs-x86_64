#include <bits/errno.h>
#include <sys/proc.h>
#include <sys/sysproc.h>
#include <sys/thread.h>
#include <sys/_wait.h>

pid_t waitpid(pid_t __pid, int *__stat_loc, int __options) {
    int     err     = 0;    // error code if an error  occurs.
    pid_t   pgid    = 0;    // process group ID in which we wait for a child.
    pid_t   pid     = 0;    // pid of process to wait for.
    int     opt     = 0;    // copy of option flags.
    proc_t  *child  = NULL; // pointer to a child process structure.

    if (__pid < -1) {
        /**
         * get status info of any child process
         * whose process group ID matches pgid.
        */
        pgid = ABS(__pid);


    } else if (__pid == -1) {
        /**
         * Get status info of any child process.
        */
       
    } else if (__pid == 0) {
        /**
         * Get status info of any child
         * whose process group ID matches
         * that of the calling process.
        */
        pgid = getpgrp();


    } else if (__pid > 0) {
        /**
         * Get status info of a child process
         * whose process ID matches pid.
        */
        pid = __pid;
    }

    return 0;
error:
    return err;
}