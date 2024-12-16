#include <bits/errno.h>
#include <lib/string.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <sys/_signal.h>

int thread_fork(thread_t *dst, thread_t *src, mmap_t *mmap) {
    uintptr_t   sp          = 0;
    uc_stack_t  uc_stack    = {0};
    vmr_t       *ustack     = NULL;
    mcontext_t  *mctx       = NULL;

    if (dst == NULL || src == NULL)
        return -EINVAL;
    
    thread_assert_locked(dst);
    thread_assert_locked(src);

    mctx = &src->t_arch.t_uctx->uc_mcontext;
    sp = mctx->rsp;

    dst->t_sched = (thread_sched_t) {
        .ts_priority        = src->t_sched.ts_priority,
        .ts_processor       = src->t_sched.ts_processor,
        .ts_timeslice       = src->t_sched.ts_timeslice,
        .ts_affinity.type   = src->t_sched.ts_affinity.type,
    };

    if ((ustack = mmap_find(mmap, sp)) == NULL) {
        return -EADDRNOTAVAIL;
    }

    uc_stack.ss_size     = __vmr_size(ustack);
    uc_stack.ss_flags    = __vmr_vflags(ustack);
    uc_stack.ss_sp       = (void *)__vmr_upper_bound(ustack);
    dst->t_arch.t_ustack = uc_stack;
    return arch_thread_fork(&dst->t_arch, &src->t_arch);
}

int thread_execve(thread_t *thread, thread_entry_t entry,
    const char *argp[], const char *envp[]) {
    int         err         = 0;
    int         argc        = 0;
    uc_stack_t  uc_stack    = {0};
    uc_stack_t  tmp_stack   = {0};
    char        **arg       = NULL;
    char        **env       = NULL;
    vmr_t       *ustack     = NULL;

    if (thread == NULL || entry == NULL)
        return -EINVAL;

    thread_assert_locked(thread);

    // TODO: implement a function to reverse this.
    if ((err = mmap_argenvcpy(thread->t_mmap, (const char **)argp,
        (const char **)envp, &arg, &argc, &env)))
        return err;

    tmp_stack = thread->t_arch.t_ustack;

    if ((err = mmap_alloc_stack(thread->t_mmap, USTACKSZ, &ustack)))
        goto error;

    uc_stack.ss_size    = __vmr_size(ustack);
    uc_stack.ss_flags   = __vmr_vflags(ustack);
    uc_stack.ss_sp      = (void *)__vmr_upper_bound(ustack);

    thread->t_arch.t_ustack = uc_stack;

    if ((err = arch_thread_execve(&thread->t_arch, entry, argc,
        (const char **)arg, (const char **)env)))
        goto error;
    return 0;
error:
    thread->t_arch.t_ustack = tmp_stack;

    //TODO: add here, a call to reverse mmap_argenvcpy()

    mmap_remove(thread->t_mmap, ustack);
    return err;
}