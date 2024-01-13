#include <bits/errno.h>
#include <lib/printk.h>
#include <sys/syscall.h>
#include <arch/x86_64/context.h>

size_t (*syscall[])() = {
    [SYS_PUTC] = (void *)sys_putc,
    [SYS_CLOSE] = (void *)sys_close,
    [SYS_UNLINK] = (void *)sys_unlink,
    [SYS_DUP] = (void *)sys_dup,
    [SYS_DUP2] = (void *)sys_dup2,
    [SYS_TRUNCATE] = (void *)sys_truncate,
    [SYS_FCNTL] = (void *)sys_fcntl,
    [SYS_IOCTL] = (void *)sys_ioctl,
    [SYS_LSEEK] = (void *)sys_lseek,
    [SYS_READ] = (void *)sys_read,
    [SYS_WRITE] = (void *)sys_write,
    [SYS_OPEN] = (void *)sys_open,
    [SYS_CREATE] = (void *)sys_create,
    [SYS_MKDIRAT] = (void *)sys_mkdirat,
    [SYS_READDIR] = (void *)sys_readdir,
    [SYS_LINKAT] = (void *)sys_linkat,
    [SYS_MKNODAT] = (void *)sys_mknodat,
    [SYS_SYNC] = (void *)sys_sync,
    [SYS_GETATTR] = (void *)sys_getattr,
    [SYS_SETATTR] = (void *)sys_setattr,
    [SYS_PARK] = (void *)sys_park,
    [SYS_UNPARK] = (void *)sys_unpark,
    [SYS_EXIT] = (void *)sys_exit,
    [SYS_GETPID] = (void *)sys_getpid,
    [SYS_GETPPID] = (void *)sys_getppid,
    [SYS_SLEEP] = (void *)sys_sleep,
    [SYS_GETTID] = (void *)sys_gettid,
    [SYS_THREAD_EXIT] = (void *)sys_thread_exit,
    [SYS_THREAD_CREATE] = (void *)sys_thread_create,
    [SYS_THREAD_JOIN] = (void *)sys_thread_join,
    [SYS_PAUSE] = (void *)sys_pause,
    [SYS_KILL] = (void *)sys_kill,
    [SYS_ALARM] = (void *)sys_alarm,
    [SYS_SIGNAL] = (void *)sys_signal,
    [SYS_SIGPROCMASK] = (void *)sys_sigprocmask,
    [SYS_SIGPENDING] = (void *)sys_sigpending,
    [SYS_SIGACTION] = (void *)sys_sigaction,
    [SYS_PTHREAD_KILL] = (void *)sys_pthread_kill,
    [SYS_SIGWAIT] = (void *)sys_sigwait,
    [SYS_PTHREAD_SIGMASK] = (void *)sys_pthread_sigmask,
    [SYS_THREAD_SELF] = (void *)sys_thread_self,
};

static int sys_syscall_ni(tf_t *tf) {
    printk("syscall(%d) not implemented\n", tf->rax);
    return -ENOSYS;
}

void do_syscall(tf_t *tf) {
    if (tf == NULL)
        return;

    if (tf->rax >= NELEM(syscall))
        tf->rax = sys_syscall_ni(tf);
    else if ((long)tf->rax < 0 || !syscall[tf->rax])
        tf->rax = sys_syscall_ni(tf);
    else
        tf->rax = (syscall[tf->rax])(tf->rdi,
        tf->rsi, tf->rdx, tf->rcx, tf->r8, tf->r9, tf->rsp);
}
