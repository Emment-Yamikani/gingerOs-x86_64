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

int argenv_cpy(char *__argv[], char *__envp[], argvenvp_t *ppargs) {
    int     err     = 0;
    usize   cnt     = 0;
    char    **argp  = NULL, **envp  = NULL, **tmp = NULL;

    if (__argv == NULL || __envp == NULL || ppargs == NULL)
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
    argvenvp_t      argv_envp   = {0};

    /**
     * Suspend execution of other threads in this process
     * to ensure they don't temper with the state of this
     * process, incase an error occurs so we can graceffuly
     * return and continue execution from where we ended.
    */
    if ((err = tgroup_suspend(current_tgroup())))
        return err;
    
    // copy the filename of the new process image.
    if (NULL == (fncpy = strdup(pathname)))
        goto error;

    /**
     * copy argv and envp
     * before switching to different address space.
    */
    if ((err = argenv_cpy((char **)argv, (char **)envp, &argv_envp)))
        goto error;

    // allocate new address space.
    if ((err = mmap_alloc(&mmap)))
        goto error;

    // switcht o new address space
    if ((err = mmap_focus(mmap, NULL)))
        goto error0;

    if ((err = proc_load(fncpy, mmap, &entry)))
        goto error;

    curproc->entry       = entry;
    curproc->mmap        = mmap;
    curproc->main_thread = current;
    curproc->flags      |= PROC_EXECED;

    return 0;
error0:
    mmap_focus(curproc->mmap, NULL);
error:
    // TODO: bring back threads we suspended

    printk("%s:%d: %s() failed, err = %d\n", __FILE__, __LINE__, __func__, err);
    return err;
}