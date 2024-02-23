#include <lib/printk.h>
#include <sys/syscall.h>
#include <arch/x86_64/context.h>
#include <sys/thread.h>
#include <sys/proc.h>

void exit(int exit_code) {
    int     err = 0;
    proc_t *parent = NULL;

    if (curproc == initproc)
        panic("initproc not allowed to exit?!\n");
    
    if ((err = thread_kill_all())) {
        panic(
            "%s:%d: proc(%d): Failed to kill all"
            " threads, error: %d",
            __FILE__, __LINE__, curproc->pid,
            thread_self(), err
        );
    }

    // cancel all pending signals.

    // close all open file descriptors.
    file_close_all();

    proc_lock(curproc);

    // abandon children to 'init'.
    proc_lock(initproc);
    if ((err = proc_abandon_children(initproc, curproc))) {
        panic(
            "%s:%d: [%d:%d]: "
            "Error abandoning children, error: %d",
            __FILE__, __LINE__, curproc->pid,
            thread_self(), err
        );
    }
    proc_unlock(initproc);

    // clean mmap.
    mmap_lock(curproc->mmap);
    if ((err = mmap_clean(curproc->mmap))) {
        panic(
            "%s:%d: [%d:%d]: "
            "Error cleaning mmap, error: %d",
            __FILE__, __LINE__, curproc->pid,
            thread_self(), err
        );
    }
    mmap_unlock(curproc->mmap);

    curproc->state      = P_ZOMBIE;
    curproc->exit_code  = exit_code;

    parent = curproc->parent;
    // broadcast event to parent process.
    proc_lock(parent);
    cond_broadcast(&parent->child_event);
    proc_unlock(parent);
    proc_unlock(curproc);

    printk("%s:%ld: %s(%d);\n", __FILE__, __LINE__, __func__, exit_code);
    thread_exit(exit_code);
}