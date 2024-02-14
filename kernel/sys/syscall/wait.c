#include <bits/errno.h>
#include <sys/proc.h>
#include <sys/sysproc.h>
#include <sys/thread.h>
#include <sys/_wait.h>

__unused
pid_t waitpid(pid_t pid, int *stat_loc, int opt) {
    int     err = 0;
    (void)stat_loc, (void)opt;

    loop() {
    switch (pid) {
    case -1: // wait for any child.
        break;
    case 0: // wait for process whose pgid == getpgrp().
        break;
    default:
        // wait for process whose pgid == abs(pid)
        if (pid < -1) {

        } else { // wait for process whose pid == pid
            
        }
        break;
    }
    }

    return 0;
}