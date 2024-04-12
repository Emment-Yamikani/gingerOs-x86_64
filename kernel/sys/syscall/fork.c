#include <arch/paging.h>
#include <bits/errno.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <mm/mmap.h>
#include <mm/kalloc.h>
#include <sys/proc.h>
#include <sys/sysproc.h>
#include <sys/thread.h>

pid_t fork(void) {
    int         err     = 0;
    pid_t       pid     = 0;
    proc_t      *child  = NULL;
    thread_t    *thread = NULL;

    if (curproc == NULL)
        return -EINVAL;
    
    proc_lock(curproc);

    if ((err = proc_alloc(curproc->name, &child)))
        return err;

    mmap_lock(proc_mmap(curproc));
    mmap_lock(proc_mmap(child));

    if ((err = proc_copy(child, curproc))) {
        mmap_unlock(proc_mmap(child));
        mmap_unlock(proc_mmap(curproc));
        goto error;
    }

    mmap_unlock(proc_mmap(curproc));

    pid     = child->pid;
    thread_lock(child->main_thread);
    thread  = thread_getref(child->main_thread);

    current_lock();
    if ((err = thread_fork(thread, current, child->mmap))) {
        current_unlock();
        thread_release(thread);
        goto error;
    }
    current_unlock();

    mmap_unlock(proc_mmap(child));

    if ((err = proc_add_child(curproc, child))) {
        thread_unlock(thread);
        proc_unlock(child);
        proc_unlock(curproc);
        goto error;
    }

    proc_unlock(child);
    proc_unlock(curproc);

    if ((err = thread_schedule(thread))) {
        thread_unlock(thread);
        goto error;
    }

    thread_unlock(thread);
    return pid;
error:
    if (child)
        proc_free(child);
    return err;
}