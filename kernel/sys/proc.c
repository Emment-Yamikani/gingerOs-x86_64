#include <lib/string.h>
#include <mm/kalloc.h>
#include <sys/proc.h>
#include <sys/session.h>

int proc_alloc(const char *path, proc_t **pref) {
    int         err     = 0;
    proc_t     *proc    = NULL;
    mmap_t     *mmap    = NULL;
    tgroup_t   *tgroup  = NULL;

    if (path == NULL || pref == NULL)
        return -EINVAL;

    if (NULL == (proc = kmalloc(sizeof *proc)))
        return -ENOMEM;

    if ((err = mmap_alloc(&mmap)))
        goto error;

    if ((err = tgroup_create(&tgroup)))
        goto error;

    memset(proc, 0, sizeof *proc);

    proc->mmap = mmap;
    proc->tgroup = tgroup;
    proc->wait = COND_INIT();
    proc->lock = SPINLOCK_INIT();

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

void proc_free(proc_t *proc);
int proc_init(proc_t *proc);