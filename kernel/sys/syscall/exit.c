#include <lib/printk.h>
#include <sys/syscall.h>
#include <arch/x86_64/context.h>
#include <sys/thread.h>
#include <sys/proc.h>

void exit(int exit_code) {
    int     err = 0;

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

    file_close_all();

    proc_lock(curproc);

    if ((err = procQ_remove(curproc))) {
        panic(
            "%s:%d: [%d:%d]: "
            "Failed to remove from"
            " the process queue, error: %d",
            __FILE__, __LINE__, curproc->pid,
            thread_self(), err
        );
    }

    // abandon children to initproc.
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

    curproc->state      = ZOMBIE;
    curproc->exit_code  = exit_code;

    // broadcast event to self and to parent.
    proc_lock(curproc->parent);
    cond_broadcast(&curproc->wait);
    cond_broadcast(&curproc->parent->wait);
    proc_unlock(curproc->parent);
    proc_unlock(curproc);

    printk("%s:%ld: %s(%d);\n", __FILE__, __LINE__, __func__, exit_code);
    thread_exit(exit_code);
}