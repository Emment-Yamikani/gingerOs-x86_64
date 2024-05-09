#include <bits/errno.h>
#include <fs/fs.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <sys/proc.h>
#include <sys/elf/elf.h>

static struct binfmt {
    int (*check)(inode_t *binary);
    int (*load)(inode_t *binary, mmap_t *mmap, thread_entry_t *entry);
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
    if (pid >= NPROC)
        return -EAGAIN;
done:
    *ref = pid;
    return 0;
}

static void proc_free_pid(pid_t pid) {
    if (pid <= 0 || pid > NPROC)
        return;
}

int procQ_remove(proc_t *proc) {
    int err = 0;

    if (proc == NULL)
        return -EINVAL;
    
    proc_assert_locked(proc);
    queue_lock(procQ);
    if ((err = queue_remove(procQ, proc)) == 0)
        proc_putref(proc);
    queue_unlock(procQ);
    return err;
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

    if (name == NULL || pref == NULL)
        return -EINVAL;

    if (NULL == (proc = kmalloc(sizeof *proc)))
        return -ENOMEM;

    if ((err = mmap_alloc(&mmap)))
        goto error;

    if ((err = thread_alloc(KSTACKSZ, THREAD_CREATE_USER, &thread)))
        goto error;

    if ((err = thread_create_group(thread)))
        goto error;

    memset(proc, 0, sizeof *proc);

    err = -ENOMEM;
    if (NULL == (proc->name = strdup(name)))
        goto error;

    if ((err = proc_alloc_pid(&proc->pid)))
        goto error;

    proc->refcnt        = 1;
    proc->mmap          = mmap;
    proc->pgroup        = proc->pid;
    proc->session       = proc->pid;
    proc->child_event   = COND_INIT();
    proc->lock          = SPINLOCK_INIT();
    proc->cred          = thread->t_cred;
    proc->fctx          = thread->t_fctx;
    proc->threads       = thread->t_tgroup;
    proc->sigdesc       = thread->t_sigdesc;
    proc->main_thread   = thread_getref(thread);

    thread->t_mmap      = mmap;
    proc_lock(proc);

    thread->t_owner = proc_getref(proc);
    mmap_unlock(mmap);

    thread_unlock(thread);

    *pref = proc;
    return 0;
error:
    if (proc->name)
        kfree(proc->name);

    if (thread)
        thread_free(thread);

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

int proc_load(const char *pathname, mmap_t *mmap, thread_entry_t *entry) {
    int         err     = 0;
    uintptr_t   pdbr    = 0;
    inode_t     *binary = NULL;
    dentry_t    *dentry = NULL;

    if (mmap == NULL || entry == NULL)
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

    for (size_t i = 0; i <= NELEM(binfmt); ++i) {
        /// check the binary image to make sure it is a valid program file.
        if ((err = binfmt[i].check(binary))) {
            iunlock(binary);
            goto error;
        }

        /// load the binary image into memory in readiness for execution.
        if ((err = binfmt[i].load(binary, mmap, entry)) == 0)
            goto commit;
    }

    /// binary file not loaded ???.
    iunlock(binary);
    goto error;
commit:
    iunlock(binary);
    /**
     * @brief close this dentry.
     * dentry->d_inode must have been opened.
     * To remain persistent.
     */
    dclose(dentry);
    return 0;
error:
    if (pdbr)
        arch_swtchvm(pdbr, NULL);

    if (dentry)
        dclose(dentry);

    return err;
}

int proc_init(const char *initpath) {
    int             err     = 0;
    uintptr_t       pdbr    = 0;
    proc_t          *proc   = NULL;
    thread_t        *thread = NULL;
    const char *argp[]      = { initpath, NULL };
    const char *envp[]      = { "MOUNT=/mnt/",
                                "PATH=/mnt/ramfs/",
                                "ROOT=/",
                                "TMPFS=/tmp/",
                                NULL
    };

    if ((err = proc_alloc(initpath, &proc)))
        goto error;

    proc_mmap_lock(proc);
    if ((err = mmap_focus(proc_mmap(proc), &pdbr))) {
        proc_mmap_unlock(proc);
        goto error;
    }

    if ((err = proc_load(initpath, proc_mmap(proc), &proc->entry))) {
        proc_mmap_unlock(proc);
        goto error;
    }

    thread_lock(proc->main_thread);
    thread = thread_getref(proc->main_thread);

    if ((err = thread_execve(thread, proc->entry, argp, envp))) {
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
    file_ctx_t *fctx    = NULL;
    cred_t      *cred   = NULL;

    if (child == NULL || parent == NULL)
        return -EINVAL;
    
    proc_assert_locked(child);
    proc_assert_locked(parent);

    if ((err = procQ_insert(child)))
        return err;

    if ((err = mmap_copy(child->mmap, parent->mmap)))
        goto error;

    fctx = current->t_fctx;
    cred = current->t_cred;

    fctx_lock(fctx);
    fctx_lock(child->fctx);

    if ((err = file_copy(child->fctx, fctx))) {
        fctx_unlock(child->fctx);
        fctx_unlock(fctx);
        goto error;
    }

    fctx_unlock(child->fctx);
    fctx_unlock(fctx);

    cred_lock(cred);
    cred_lock(child->cred);
    if ((err = cred_copy(child->cred, cred))) {
        cred_unlock(child->cred);
        cred_unlock(cred);
        return err;
    }
    cred_unlock(child->cred);
    cred_unlock(cred);

    child->entry    = parent->entry;
    child->pgroup   = parent->pgroup;
    child->session  = parent->session;
    child->exit_code= parent->exit_code;
    child->parent   = proc_getref(curproc);

    return 0;
error:
    /// TODO: Reverse mmap_clone(),
    /// but i think it will be handled by proc_free().
    return err;
}

/********************************************************************************/
/***********************    PROCESS QUEUE HELPERS    ****************************/
/********************************************************************************/

int proc_add_child(proc_t *parent, proc_t *child) {
    int err = 0;

    if (parent == NULL || child == NULL)
        return -EINVAL;
    
    proc_assert_locked(child);
    proc_assert_locked(parent);

    queue_lock(&parent->children);
    if ((err = enqueue(&parent->children, (void *)child, 1, NULL)) == 0)
        proc_getref(child);
    queue_unlock(&parent->children);

    return err;
}

int proc_remove_child(proc_t *parent, proc_t *child) {
     int err = 0;

    if (parent == NULL || child == NULL)
        return -EINVAL;
    
    proc_assert_locked(child);
    proc_assert_locked(parent);

    queue_lock(&parent->children);
    if ((err = queue_remove(&parent->children, (void *)child)) == 0)
        proc_putref(child);
    queue_unlock(&parent->children);

    return err;
}

int proc_abandon_children(proc_t *new_parent, proc_t *old_parent) {
    int             err         = 0;
    proc_t          *child      = NULL;
    queue_node_t    *next_node  = NULL;

    if (new_parent == NULL || old_parent == NULL)
        return -EINVAL;

    proc_assert_locked(old_parent);
    proc_assert_locked(new_parent);

    queue_lock(&new_parent->children);
    queue_lock(&old_parent->children);
    
    forlinked (node, old_parent->children.head, next_node) {
        next_node = node->next;
        child = (proc_t *)node->data;
        
        proc_lock(child);
        
        if ((err = enqueue(&new_parent->children, (void *)child, 1, NULL))) {
            proc_unlock(child);
            queue_unlock(&old_parent->children);
            queue_unlock(&new_parent->children);
            return err;
        }
        
        if ((err = queue_remove(&old_parent->children, (void *)child))) {
            queue_remove(&new_parent->children, child);
            proc_unlock(child);
            queue_unlock(&old_parent->children);
            queue_unlock(&new_parent->children);
            return err;
        }

        proc_putref(child->parent);
        child->parent = proc_getref(new_parent);
        // printk("Child process abandoned.\n");
        proc_unlock(child);
    }
    
    queue_unlock(&old_parent->children);
    queue_unlock(&new_parent->children);
    return 0;
}
