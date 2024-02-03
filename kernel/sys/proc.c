#include <bits/errno.h>
#include <fs/fs.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <sys/proc.h>
#include <sys/elf/elf.h>
#include <sys/session.h>

static struct binfmt {
    int (*check)(inode_t *binary);
    int (*load)(inode_t *binary, proc_t *proc);
} binfmt[] = {
    { // ELF loader
        .check = binfmt_elf_check,
        .load = binfmt_elf_load,
    },
};

proc_t *initproc = NULL;

queue_t *procQ = QUEUE_NEW();

// bucket to hold free'd PIDs.
static queue_t *procIDs = QUEUE_NEW();

/**
 * For process ID alocation.
 * to be moved to sys/proc.c
 */
static atomic_t pids = {0};

static int proc_alloc_pid(pid_t *ref) {
    int     err = 0;
    pid_t   pid = 0;

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

int procQ_get(pid_t pid, proc_t **ppp) {
    proc_t *proc = NULL;
    
    queue_lock(procQ);
    forlinked(node, procQ->head, node->next) {
        proc = node->data;
        proc_lock(proc);
        if (proc->pid == pid) {
            *ppp = proc_getref(proc);
            queue_unlock(procQ);
            return 0;
        }
        proc_unlock(proc);
    }
    queue_unlock(procQ);

    return -ESRCH;
}

int procQ_insert(proc_t *proc) {
    int err = 0;
    
    if (proc == NULL)
        return -EINVAL;
    
    queue_lock(procQ);
    if ((err = enqueue(procQ, (void *)proc_getref(proc), 1, NULL)))
        proc->refcnt--;
    queue_unlock(procQ);

    return err;
}

void procQ_remove_bypid(pid_t pid) {
    proc_t *proc = NULL;
    queue_lock(procQ);
    forlinked(node, procQ->head, node->next) {
        proc = node->data;
        proc_lock(proc);
        if (proc->pid == pid) {
            proc_free(proc);
            queue_unlock(procQ);
            return;
        }
        proc_unlock(proc);
    }
    queue_unlock(procQ);
}

int procQ_search_bypid(pid_t pid, proc_t **ref) {
    proc_t *proc = NULL;
    queue_lock(procQ);
    forlinked(node, procQ->head, node->next) {
        proc = node->data;
        proc_lock(proc);
        if (proc->pid == pid) {
            if (ref)
                *ref = proc_getref(proc);
            else
                proc_unlock(proc);
            queue_unlock(procQ);
            return 0;
        }
        proc_unlock(proc);
    }
    queue_unlock(procQ);

    return -ESRCH;
}

int procQ_search_bypgid(pid_t pgid, proc_t **ref) {
    proc_t *proc = NULL;
    queue_lock(procQ);
    forlinked(node, procQ->head, node->next) {
        proc = node->data;
        proc_lock(proc);
        if (proc->pgroup == pgid) {
            if (ref)
                *ref = proc_getref(proc);
            else
                proc_unlock(proc);
            queue_unlock(procQ);
            return 0;
        }
        proc_unlock(proc);
    }
    queue_unlock(procQ);

    return -ESRCH;
}

int proc_alloc(const char *name, proc_t **pref) {
    int         err     = 0;
    proc_t     *proc    = NULL;
    mmap_t     *mmap    = NULL;
    thread_t   *thread  = NULL;
    tgroup_t   *tgroup  = NULL;

    if (name == NULL || pref == NULL)
        return -EINVAL;

    if (NULL == (proc = kmalloc(sizeof *proc)))
        return -ENOMEM;

    if ((err = mmap_alloc(&mmap)))
        goto error;

    if ((err = tgroup_create(&tgroup)))
        goto error;

    if ((err = thread_alloc(KSTACKSZ, THREAD_USER, &thread)))
        goto error;

    if ((err = tgroup_add_thread(tgroup, thread)))
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
    proc->pgroup = proc->pid;
    proc->session= proc->pid;
    proc->wait   = COND_INIT();
    proc->lock   = SPINLOCK_INIT();

    thread->t_mmap = mmap;
    proc_lock(proc);

    thread->t_owner = proc_getref(proc);
    mmap_unlock(mmap);

    thread_unlock(thread);
    tgroup_unlock(tgroup);

    *pref = proc;
    return 0;
error:
    if (proc->name)
        kfree(proc->name);

    if (thread)
        thread_free(thread);

    if (tgroup)
        tgroup_destroy(tgroup);

    if (mmap)
        mmap_free(mmap);

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

        if (proc_mmap(proc))
            mmap_free(proc_mmap(proc));
        
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

int proc_load(const char *pathname, proc_t *proc, proc_t **ref) {
    int         err     = 0;
    uintptr_t   pdbr    = 0;
    int         newproc = !proc;
    inode_t     *binary = NULL;
    dentry_t    *dentry = NULL;

    if (proc == NULL && ref == NULL)
        return -EINVAL;

    if ((err = vfs_lookup(pathname, NULL, O_EXEC | O_RDONLY, 0, 0, &dentry)))
        goto error;

    binary = dentry->d_inode;

    err = -ENOENT;
    if (binary == NULL)
        goto error;

    /// check file  type and only execute an appropriate file type.
    ilock(binary);
    if (IISDEV(binary)) {
        err = -ENOEXEC;
        iunlock(binary);
        goto error;
    } else if (IISDIR(binary)) {
        err = -EISDIR;
        iunlock(binary);
        goto error;
    } else if (IISFIFO(binary)) {
        err = -ENOEXEC;
        iunlock(binary);
        goto error;
    } else if (IISSYM(binary)) {
        err = -ENOEXEC;
        iunlock(binary);
        goto error; 
    }
    iunlock(binary);

    if (newproc) {
        if ((err = proc_alloc(pathname, &proc)))
            goto error;

        proc_mmap_lock(proc);
        if ((err = mmap_focus(proc_mmap(proc), &pdbr))) {
            proc_mmap_unlock(proc);
            goto error;
        }
    } else {
        if ((err = thread_kill_all()))
            goto error;
        proc_mmap_assert_locked(proc);
    }

    ilock(binary);
    for (size_t i = 0; i <= NELEM(binfmt); ++i) {
        /// check the binary image to make sure it is a valid program file.
        if ((err = binfmt[i].check(binary))) {
            iunlock(binary);
            goto error;
        }

        /// load the binary image into memory in readiness for execution.
        if ((err = binfmt[i].load(binary, proc)) == 0)
            goto commit;
    }

    /// binary file not loaded ???.
    iunlock(binary);
    goto error;

commit:
    iunlock(binary);

    /// switch back to the previous PDBR.
    arch_swtchvm(pdbr, NULL);

    /// mmap was lock in this function and so must be release.
    if (newproc)
        proc_mmap_unlock(proc);

    /**
     * @brief close this dentry.
     * dentry->d_inode must have been opened.
     * To remain persistent.
     */
    dclose(dentry);

    if (ref) *ref = proc;

    return 0;
error:
    if (pdbr)
        arch_swtchvm(pdbr, NULL);

    if (dentry)
        dclose(dentry);

    return err;
}

int proc_init(const char *initpath) {
    int         err     = 0;
    uintptr_t   pdbr    = 0;
    proc_t      *proc   = NULL;
    thread_t    *thread = NULL;

    const char *argp[] = { initpath, NULL };
    const char *envp[] = { "MOUNT=/mnt/", "PATH=/mnt/ramfs/", "ROOT=/", "TMPFS=/tmp/", NULL };

    if ((err = proc_load(initpath, NULL, &proc)))
        goto error;

    proc_mmap_lock(proc);

    if ((err = mmap_focus(proc_mmap(proc), &pdbr))) {
        proc_mmap_unlock(proc);
        goto error;
    }

    proc_tgroup_lock(proc);
    if ((err = tgroup_getmain(proc_tgroup(proc), &thread))) {
        proc_tgroup_unlock(proc);
        goto error;
    }
    proc_tgroup_unlock(proc);

    if ((err = thread_execve(proc, thread, proc->entry, argp, envp))) {
        thread_release(thread);
        goto error;
    }

    proc_mmap_unlock(proc);

    if ((err = thread_schedule(thread))) {
        thread_release(thread);
        goto error;
    }

    thread_release(thread);

    if ((err = procQ_insert(proc))) {
        proc_unlock(proc);
        goto error;
    }

    initproc = proc;
    proc_unlock(proc);

    return 0;
error:
    if (proc)
        proc_free(proc);
    return err;
}

int proc_copy(proc_t *child, proc_t *parent) {
    int         err     = 0;

    if (child == NULL || parent == NULL)
        return -EINVAL;
    
    proc_assert_locked(child);
    proc_assert_locked(parent);

    if ((err = procQ_insert(child)))
        return err;

    if ((err = mmap_copy(child->mmap, parent->mmap)))
        goto error;

    child->exit     = parent->exit;
    child->entry    = parent->entry;
    child->pgroup   = parent->pgroup;
    child->session  = parent->session;
    child->parent   = proc_getref(curproc);

    return 0;
error:
    return err;
}

int proc_search_by_pgid(pid_t pgid, proc_t **ref) {
    (void)pgid, (void)ref;
    return -ESRCH;
}