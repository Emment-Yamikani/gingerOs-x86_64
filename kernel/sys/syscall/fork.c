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

    pid = child->pid;
    tgroup_lock(child->tgroup);
    if ((err = tgroup_getmain(child->tgroup, &thread))) {
        tgroup_unlock(child->tgroup);
        goto error;
    }
    tgroup_unlock(child->tgroup);

    current_lock();
    if ((err = thread_fork(thread, current, child->mmap))) {
        current_unlock();
        thread_unlock(thread);
        goto error;
    }
    current_unlock();

    mmap_unlock(proc_mmap(child));

    queue_lock(&curproc->children);
    if ((err = enqueue(&curproc->children, (void *)child, 1, NULL))) {
        queue_unlock(&curproc->children);
        proc_unlock(child);
        proc_unlock(curproc);
        thread_unlock(thread);
        goto error;
    }
    queue_unlock(&curproc->children);

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