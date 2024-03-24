#include <arch/thread.h>
#include <arch/paging.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <mm/mmap.h>
#include <sys/_signal.h>
#include <sys/thread.h>

void arch_thread_exit(uintptr_t exit_code) {
#if defined (__x86_64__)
    x86_64_thread_exit(exit_code);
#endif
}

int arch_uthread_init(arch_thread_t *arch, thread_entry_t entry, void *arg) {
#if defined __x86_64__
    return x86_64_uthread_init(arch, entry, arg);
#endif
}

int arch_kthread_init(arch_thread_t *arch, thread_entry_t entry, void *arg) {
#if defined __x86_64__
    return x86_64_kthread_init(arch, entry, arg);
#endif
}

char ***arch_execve_copy(char *_argp[], char *_envp[]) {
    int argc = 0;
    int envc = 0;
    char **argp = NULL;
    char **envp = NULL;
    char ***ptr = kcalloc(3, sizeof(char **));

    if (_argp) foreach (token, _argp) argc++;

    if (_envp) foreach (token, _envp) envc++;

    argp = kcalloc(argc + 1, sizeof(char *));
    argp[argc] = NULL;
    
    envp = kcalloc(envc + 1, sizeof(char *));
    envp[envc] = NULL;

    if (_argp) {
        foreach (arg, _argp) {
            argc--;
            argp[argc] = strdup(_argp[argc]);
        }
    }

    if (_envp) {
        foreach (env, _envp) {
            envc--;
            envp[envc] = strdup(_envp[envc]);
        }
    }

    ptr[0] = argp;
    ptr[1] = envp;
    ptr[2] = NULL;

    return ptr;
}

void arch_exec_free_copy(char ***arg_env) {
    if (!arg_env) return;
    foreach (list, arg_env) foreach (arg, list) kfree(arg);
    kfree(arg_env);
}

int arch_thread_execve(arch_thread_t *arch, thread_entry_t entry, int argc, const char *argp[], const char *envp[]) {
#if defined __x86_64__
    return x86_64_thread_execve(arch, entry, argc, argp, envp);
#endif
}

int arch_thread_setkstack(arch_thread_t *arch) {
#if defined __x86_64__
    return x86_64_thread_setkstack(arch);
#endif
}

int arch_thread_fork(arch_thread_t *dst, arch_thread_t *src) {
#if defined __x86_64__
    return x86_64_thread_fork(dst, src);
#endif
}