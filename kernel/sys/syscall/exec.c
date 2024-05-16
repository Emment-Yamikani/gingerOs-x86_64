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

int copy_argenv(char *__argv[], char *__envp[], argvenvp_t *ppargs) {
    int     err     = 0;
    usize   cnt     = 0;
    char    **tmpv  = NULL;
    char    **argp  = NULL;
    char    **envp  = NULL;

    if (ppargs == NULL)
        return -EINVAL;

    foreach(arg, __argv) {
        if (NULL == (tmpv = (char **)krealloc(argp, (cnt + 2) * sizeof (char **))))
            goto error;
        argp = tmpv;
        if (NULL == (argp[cnt] = strdup(arg)))
            goto error;
        argp[++cnt] = NULL;
    }

    cnt = 0;
    foreach(env, __envp) {
        if (NULL == (tmpv = (char **)krealloc(envp, (cnt + 2) * sizeof (char **))))
            goto error;
        envp = tmpv;
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
    atomic_t        flags       = 0;
    argvenvp_t      args        = {0};
    thread_t        *thread     = NULL;
    char            *binary     = NULL;
    mmap_t          *mmap       = NULL;
    queue_t         *sigqueue   = NULL;
    sig_desc_t      *sigdesc    = NULL;

    current_lock();
    flags   = current->t_flags;
    current_unlock();
    /**
     * Suspend execution of other threads in this process
     * to ensure they don't temper with the state of this
     * process, incase an error occurs so we can graceffuly
     * return and continue execution from where we ended.*/
    if ((err = tgroup_suspend(current_tgroup())))
        return err;

    // copy the filename of the new process image.
    if (NULL == (binary = strdup(pathname))) {
        err = -ENOMEM;
        goto error;
    }

    /**
     * copy argv and envp
     * before switching to different address space.
    */
    if ((err = copy_argenv((char **)argv, (char **)envp, &args)))
        goto error;

    // allocate new address space.
    if ((err = mmap_alloc(&mmap)))
        goto error;

    // switch of new address space.
    if ((err = mmap_focus(mmap, NULL)))
        goto error0;

    if ((err = proc_load(binary, mmap, &entry)))
        goto error;

    /**
     * @brief Construct a thread
     * new process image will run in a new thread.
     * POSIX says "the thread ID of the initial thread in
     * the new process image is undefined".
     * So, new thread will become the main thread in the process.
     */
    if ((err = thread_alloc(KSTACKSZ, THREAD_CREATE_USER, &thread)))
        goto error;

    thread->t_mmap = mmap;
    thread_setmain(thread);
    thread_setlast(thread);
    thread_set_killexcept(thread);
    assert_msg(0 == (err = thread_join_group(thread)),
        "%s:%d: Failed to joing tgroup. error: %d\n", __FILE__, __LINE__, err);

    thread->t_entry  = entry;
    thread->t_sched  = current->t_sched;
    sigdesc          = current->t_sigdesc;
    thread->t_sigmask= current->t_sigmask;

    if ((err = thread_execve(
        thread, entry,
        (const char **)args.argv,
        (const char **)args.envp
    ))) goto error;

    proc_lock(curproc);
    curproc->mmap       = mmap;
    curproc->entry      = entry;
    curproc->main_thread= thread;
    curproc->flags     |= PROC_EXECED;
    proc_unlock(curproc);

    mmap_unlock(mmap);

    // reset global signals.
    sigdesc_lock(sigdesc);
    sigqueue = sigdesc->sig_queue;
    for (int signo = 0; signo < NSIG; ++signo) {
        queue_lock(&sigqueue[signo]);
        queue_foreach(siginfo_t *, siginfo, &sigqueue[signo]) {
            kfree(siginfo); // free the pending signal description.
        }
        queue_unlock(&sigqueue[signo]);
    }

    // reset all signal actions.
    for (int signo = 0; signo < NSIG; ++signo) {
        sigdesc->sig_action[signo].sa_flags     = 0;
        sigdesc->sig_action[signo].sa_handler   = NULL;
        sigdesc->sig_action[signo].sa_sigaction = NULL;
        sigemptyset(&sigdesc->sig_action[signo].sa_mask);
    }

    // reset the global signal mask.
    sigemptyset(&sigdesc->sig_mask);
    sigdesc_unlock(sigdesc);

    thread_unlock(thread);

    /**
     * @brief Kill all suspended threads
     * except current and those with at least except_flags set.
     */
    current_tgroup_lock();
    assert_msg(0 == (err = tgroup_kill_thread(
                         current->t_tgroup, -1, THREAD_KILLEXCEPT, 1)),
               "%s%d: Failed to kill suspended threads. error: %d.\n", __FILE__, __LINE__, err);
    current_tgroup_unlock();

    thread_lock(thread);
    assert_msg(0 == (err = thread_schedule(thread)),
        "%s:%d: thread_schedule() failed, error: %d\n", __FILE__, __LINE__, err);
    thread_unset_killexcept(thread);
    thread_unlock(thread);

    thread_exit(0);
    return 0;
error0:
    mmap_focus(curproc->mmap, NULL);
error:
    tgroup_unsuspend(current->t_tgroup);

    if (binary)
        kfree(binary);
    
    if (args.argv)
        tokens_free(args.argv);

    if (args.envp)
        tokens_free(args.envp);

    if (mmap)
        mmap_free(mmap);

    current_lock();
    current->t_flags = flags;
    current_unlock();

    printk("%s:%d: %s() failed, err = %d\n", __FILE__, __LINE__, __func__, err);
    return err;
}