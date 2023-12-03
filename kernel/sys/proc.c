#include <lib/string.h>
#include <mm/kalloc.h>
#include <sys/proc.h>
#include <sys/session.h>

// bucket to hold free'd PIDs.
static queue_t *procIDs = QUEUE_NEW();

/**
 * For process ID alocation.
 * to be moved to sys/proc.c
 */
static atomic_t pids = {0};

static int proc_alloc_pid(pid_t *ref) {
    int err = 0;
    pid_t pid = 0;

    if (ref == NULL)
        return -EINVAL;
    
    queue_lock(procIDs);
    err = dequeue(procIDs, (void **)&pid);
    queue_unlock(procIDs);

    if (err == 0) goto done;

    pid = (pid_t) atomic_inc_fetch(&pids);
    if (pid > NPROC)
        return -EAGAIN;
done:
    *ref = pid;
    return 0;
}

static void proc_free_pid(pid_t pid) {
    if (pid <= 0 || pid > NPROC)
        return;
}

int proc_alloc(const char *name, proc_t **pref) {
    int         err     = 0;
    proc_t     *proc    = NULL;
    mmap_t     *mmap    = NULL;
    tgroup_t   *tgroup  = NULL;

    if (name == NULL || pref == NULL)
        return -EINVAL;

    if (NULL == (proc = kmalloc(sizeof *proc)))
        return -ENOMEM;

    if ((err = mmap_alloc(&mmap)))
        goto error;

    if ((err = tgroup_create(&tgroup)))
        goto error;

    memset(proc, 0, sizeof *proc);

    err = -ENOMEM;
    if (NULL == (proc->name = strdup(name)))
        goto error;

    if ((err = proc_alloc_pid(&proc->pid)))
        goto error;

    proc->refcnt = 1;
    proc->mmap   = mmap;
    proc->tgroup = tgroup;
    proc->wait   = COND_INIT();
    proc->lock   = SPINLOCK_INIT();

    proc_lock(proc);
    mmap_unlock(mmap);

    *pref = proc;
    return 0;
error:
    if (mmap)
        mmap_free(mmap);
    if (tgroup)
        tgroup_destroy(tgroup);
    if (proc)
        kfree(proc);
    return err;
}

void proc_free(proc_t *proc) {
    if (proc == NULL)
        return;
    
    if (curproc == proc)
        panic("ERROR: process cannot destroy self\n");
    
    if (!proc_islocked(proc))
        proc_lock(proc);
    

    proc->refcnt--;

    /**
     * @brief If the refcnt of proc is zero(0),
     * release all resources allocated to it.
     * However, if at all we have some thread(s) sleeping(wating) for
     * a broadcast on 'proc->wait' to be fired, desaster may unfold.
     */
    if (proc->refcnt <= 0) {
        queue_lock(&proc->children);
        queue_flush(&proc->children);
        queue_unlock(&proc->children);

        if (proc->mmap)
            mmap_free(proc->mmap);
        
        if (proc->tgroup)
            tgroup_destroy(proc->tgroup);
        
        if (proc->name)
            kfree(proc->name);

        proc_free_pid(proc->pid);

        proc_unlock(proc);

        /// TODO: a solution to the problem above
        /// may be to kill the thread(s) waiting
        /// to avoid access to resources that have already been released.
        kfree(proc);
        return;
    }

    proc_unlock(proc);
}

int proc_init(proc_t *proc) {
    int err = 0;

    if (proc == NULL || curproc == proc)
        return -EINVAL;

    mmap_switch()
}