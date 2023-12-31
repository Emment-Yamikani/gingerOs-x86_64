#include <bits/errno.h>
#include <lib/printk.h>
#include <sys/syscall.h>
#include <sys/thread.h>
#include <fs/file.h>
#include <arch/x86_64/context.h>
#include <sys/sleep.h>
#include <sys/_signal.h>

void sys_putc(int c) {
    printk("%c", c);
}

ssize_t  sys_read(int fd, void *buf, size_t sz) {
    return read(fd, buf, sz);
}

ssize_t  sys_write(int fd, void *buf, size_t sz) {
    return write(fd, buf, sz);
}

off_t    sys_lseek(int fd, off_t off, int whence) {
    return lseek(fd, off, whence);
}

int      sys_open(const char *pathname, int oflags, int mode) {
    return open(pathname, oflags, mode);
}

int      sys_close(int fd) {
    return close(fd);
}

int      sys_unlink(int fd) {
    return unlink(fd);
}

int      sys_dup(int fd) {
    return dup(fd);
}

int      sys_dup2(int fd1, int fd2) {
    return dup2(fd1, fd2);
}

int      sys_truncate(int fd, off_t length) {
    return truncate(fd, length);
}

int      sys_fcntl(int fd, int cmd, void *argp) {
    return fcntl(fd, cmd, argp);
}

int      sys_ioctl(int fd, int req, void *argp) {
    return ioctl(fd, req, argp);
}

int      sys_create(int fd, const char *filename, mode_t mode) {
    return create(fd, filename, mode);
}

int      sys_mkdirat(int fd, const char *filename, mode_t mode) {
    return mkdirat(fd, filename, mode);
}

ssize_t  sys_readdir(int fd, off_t off, void *buf, size_t count) {
    return readdir(fd, off, buf, count);
}

int      sys_linkat(int fd, const char *oldname, const char *newname) {
    return linkat(fd, oldname, newname);
}

int      sys_mknodat(int fd, const char *filename, mode_t mode, int devid) {
    return mknodat(fd, filename, mode, devid);
}

int      sys_sync(int fd) {
    return sync(fd);
}

int      sys_getattr(int fd, void *attr) {
    return getattr(fd, attr);
}

int      sys_setattr(int fd, void *attr) {
    return setattr(fd, attr);
}

int sys_park(void) {
    return park();
}

int sys_unpark(tid_t tid) {
    return unpark(tid);
}


void     sys_exit(int exit_code) {
    exit(exit_code);
}

pid_t    sys_getpid(void) {
    return getpid();
}

pid_t    sys_getppid(void) {
    return getppid();
}

long      sys_sleep(long seconds) {
    return sleep(seconds);
}

tid_t    sys_gettid(void) {
    return thread_gettid(current);
}

void     sys_thread_exit(int exit_code) {
    return thread_exit(exit_code);
}

int      sys_thread_create(tid_t *ptidp, void *attr, thread_entry_t entry, void *arg) {
    int err = 0;
    thread_t *thread = NULL;

    if (entry == NULL || ptidp == NULL)
        return -EINVAL;
    
    if ((err = thread_create(attr, entry, arg, &thread)))
        return err;
    
    *ptidp = thread_gettid(thread);
    thread_release(thread);

    return 0;
}

int      sys_thread_join(tid_t tid, void **retval) {
    return thread_join(tid, NULL, retval);
}

int sys_pause(void) {
    return pause();
}
int sys_kill(pid_t pid, int signo) {
    return kill(pid, signo);
}
unsigned long sys_alarm(unsigned long sec) {
    return alarm(sec);
}

sigfunc_t sys_signal(int signo, sigfunc_t func) {
    return signal(signo, func);
}

int sys_sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset) {
    return sigprocmask(how, set, oset);
}

int sys_sigpending(sigset_t *set) {
    return sigpending(set);
}

int sys_sigaction(int signo, const sigaction_t *restrict act, sigaction_t *restrict oact) {
    return sigaction(signo, act, oact);
}

int sys_pthread_kill(tid_t thread, int signo) {
    return pthread_kill(thread, signo);
}

int sys_sigwait(const sigset_t *restrict set, int *restrict signop) {
    return sigwait(set, signop);
}

int sys_pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset) {
    return pthread_sigmask(how, set, oset);
}