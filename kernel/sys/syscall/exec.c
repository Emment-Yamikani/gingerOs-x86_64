#include <bits/errno.h>
#include <mm/kalloc.h>
#include <mm/mmap.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/sysproc.h>

typedef struct {
    char **argv;
    char **envp;
} argvenvp_t;

int copy_argsenv(char *__argv[], char *__envp[], argvenvp_t *ppargs) {
    int     err     = 0;
    usize   cnt     = 0;
    char    **argp  = NULL, **envp  = NULL, **tmp = NULL;

    if (ppargs == NULL)
        return -EINVAL;

    foreach(arg, __argv) {
        if (NULL == (tmp = (char **)krealloc(argp, (cnt + 2) * sizeof (char **))))
            goto error;
        argp = tmp;
        if (NULL == (argp[cnt] = strdup(arg)))
            goto error;
        argp[++cnt] = NULL;
    }

    cnt = 0;
    foreach(env, __envp) {
        if (NULL == (tmp = (char **)krealloc(envp, (cnt + 2) * sizeof (char **))))
            goto error;
        envp = tmp;
        if (NULL == (envp[cnt] = strdup(env)))
            goto error;
        envp[++cnt] = NULL;
    }

    *ppargs = (argvenvp_t) {
        .argv = argp,
        .envp = envp
    };

    return 0;
error:
    if (argp)
        tokens_free(argp);
    if (envp)
        tokens_free(envp);
    printk("%s:%d: failed to copy argv[] and envp[], err = %d\n", __FILE__, __LINE__, err);
    return -ENOMEM;
}

int execve(const char *pathname, char *const argv[], char *const envp[]) {
    int             err         = 0;
    thread_entry_t  entry       = 0;
    mmap_t          *mmap       = NULL;
    char            *fncpy      = NULL;
    arch_thread_t   *arch       = NULL;
    arch_thread_t   arch_cpy    = {0};
    argvenvp_t      args        = {0};

    /**
     * Suspend execution of other threads in this process
     * to ensure they don't temper with the state of this
     * process, incase an error occurs so we can graceffuly
     * return and continue execution from where we ended.
    */
    if ((err = tgroup_suspend(current_tgroup())))
        return err;
    
    err = -ENOMEM;
    // copy the filename of the new process image.
    if (NULL == (fncpy = strdup(pathname)))
        goto error;

    /**
     * copy argv and envp
     * before switching to different address space.
    */
    if ((err = copy_argsenv((char **)argv, (char **)envp, &args)))
        goto error;

    // allocate new address space.
    if ((err = mmap_alloc(&mmap)))
        goto error;

    // switcht o new address space
    if ((err = mmap_focus(mmap, NULL)))
        goto error0;

    if ((err = proc_load(fncpy, mmap, &entry)))
        goto error;

    arch = &current->t_arch;
    memcpy(&arch_cpy, arch, sizeof *arch);    

    current_lock();
    current->t_mmap      = mmap;
    err = thread_execve(
        current,
        entry,
        (const char **)args.argv,
        (const char **)args.envp);
    
    if (err != 0) {
        current->t_mmap = curproc->mmap;
        current_unlock();
        goto error;
    }
    current_unlock();

    proc_lock(curproc);
    curproc->entry       = entry;
    curproc->mmap        = mmap;
    curproc->main_thread = current;
    curproc->flags      |= PROC_EXECED;
    proc_unlock(curproc);

    return 0;
error0:
    mmap_focus(curproc->mmap, NULL);
error:
    // TODO: bring back threads we suspended

    printk("%s:%d: %s() failed, err = %d\n", __FILE__, __LINE__, __func__, err);
    return err;
}