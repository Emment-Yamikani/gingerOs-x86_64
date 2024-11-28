#include <bits/errno.h>
#include <lib/printk.h>
#include <sys/syscall.h>
#include <sys/thread.h>
#include <fs/file.h>
#include <arch/x86_64/context.h>
#include <sys/sleep.h>
#include <sys/_signal.h>
#include <sys/mman/mman.h>
#include <sys/sysprot.h>
#include <sys/sysproc.h>
#include <mm/pmm.h>
#include <sys/_wait.h>
#include <sys/_time.h>
#include <sys/_utsname.h>
#include <mm/kalloc.h>
#include <dev/pty.h>
#include <sys/poll.h>

void sys_kputc(int c) {
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

int      sys_openat(int fd, const char *pathname, int oflags, mode_t mode) {
    return openat(fd, pathname, oflags, mode);
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

int      sys_create(const char *filename, mode_t mode) {
    return create(filename, mode);
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

int      sys_mkdir(const char *filename, mode_t mode) {
    return mkdir(filename, mode);
}

int      sys_mknod(const char *filename, mode_t mode, int devid) {
    return mknod(filename, mode, devid);
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

int      sys_fstat(int fildes, struct stat *buf) {
    return fstat(fildes, buf);
}

int      sys_stat(const char *restrict path, struct stat *restrict buf) {
    return stat(path, buf);
}

int      sys_lstat(const char *restrict path, struct stat *restrict buf) {
    return lstat(path, buf);
}

int      sys_fstatat(int fd, const char *restrict path, struct stat *restrict buf, int flag) {
    return fstatat(fd, path, buf, flag);
}

int     sys_pipe(int fds[2]) {
    return pipe(fds);
}

int      sys_uname(struct utsname *name) {
    return uname(name);
}

int      sys_chown(const char *pathname, uid_t uid, gid_t gid) {
    return chown(pathname, uid, gid);
}

int      sys_fchown(int fd, uid_t uid, gid_t gid) {
    return fchown(fd, uid, gid);
}

mode_t   sys_umask(mode_t cmask) {
    return umask(cmask);
}

int      sys_isatty(int fd) {
    return isatty(fd);
}

/** @brief PROTECTION */

uid_t sys_getuid(void) {
    return getuid();
}

uid_t sys_geteuid(void) {
    return geteuid();
}

gid_t sys_getegid(void) {
    return getegid();
}

gid_t sys_getgid(void) {
    return getgid();
}

int sys_setuid(uid_t uid) {
    return setuid(uid);
}

int sys_seteuid(uid_t euid) {
    return seteuid(euid);
}

int sys_setegid(gid_t egid) {
    return setegid(egid);
}

int sys_setgid(gid_t gid) {
    return setgid(gid);
}

int sys_getcwd(char *buf, size_t size) {
    int err = 0;
    char *buffer = NULL;

    if (buf == NULL)
        return -EFAULT;
    
    if (size == 0)
        return -EINVAL;
    
    if (NULL == (buffer = (char *)kmalloc(size)))
        return -ENOMEM;
    

    if ((err = getcwd(buffer, size))) {
        kfree(buffer);
        return err;
    }

    err = copy_to_user(buf, buffer, size);

    kfree(buffer);

    return err;
}

int sys_chdir(const char *path) {
    return chdir(path);
}


int sys_park(void) {
    return park();
}

int sys_unpark(tid_t tid) {
    return unpark(tid);
}

pid_t sys_fork(void) {
    return fork();
}

pid_t sys_waitpid(pid_t __pid, int *__stat_loc, int __options) {
    return waitpid( __pid, __stat_loc, __options);
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

int sys_execve(const char *pathname, char *const argv[], char *const envp[]) {
    return execve(pathname, argv, envp);
}

pid_t   sys_getsid(pid_t pid) {
    return getsid(pid);
}

pid_t   sys_setsid(void) {
    return setsid();
}

pid_t   sys_getpgrp(void) {
    return getpgrp();
}

pid_t   sys_setpgrp(void) {
    return setpgrp();
}

pid_t   sys_getpgid(pid_t pid) {
    return getpgid( pid);
}

int     sys_setpgid(pid_t pid, pid_t pgid) {
    return setpgid( pid, pgid);
}

long      sys_sleep(long seconds) {
    return sleep(seconds);
}

tid_t    sys_gettid(void) {
    return gettid();
}

void     sys_thread_exit(int exit_code) {
    return thread_exit(exit_code);
}

int      sys_thread_create(tid_t *ptidp, void *attr, thread_entry_t entry, void *arg) {
    int         err     = 0;
    thread_t    *thread = NULL;

    if (entry == NULL)
        return -EINVAL;
    
    if ((err = thread_create(attr, entry, arg, &thread)))
        return err;
    
    if (ptidp)
        *ptidp = thread_gettid(thread);
    
    err = thread_schedule(thread);

    thread_release(thread);

    return err;
}

int sys_thread_join(tid_t tid, void **retval) {
    return thread_join(tid, NULL, retval);
}

tid_t sys_thread_self(void) {
    return sys_gettid();
}

void sys_thread_yield(void) {
    thread_yield();
}

int sys_set_thread_area(void *addr __unused) {
    return -ENOSYS;
}

int sys_get_thread_area(void *addr __unused) {
    return -ENOSYS;
}

int sys_pause(void) {
    return pause();
}

int sys_raise(int signo) {
    return raise(signo);
}

int sys_kill(pid_t pid, int signo) {
    return kill(pid, signo);
}

unsigned sys_alarm(unsigned sec) {
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

int sys_sigsuspend(const sigset_t *mask __unused) {
    return -ENOSYS;
}

int sys_sigaltstack(const stack_t *restrict ss __unused, stack_t *restrict old_ss __unused) {
    return -ENOSYS;
}

void sys_sigreturn() {
    return;
}

void *sys_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off) {
    return mmap(addr, len, prot, flags, fd, off);
}

int sys_munmap(void *addr, size_t len) {
    return munmap(addr, len);
}

int sys_mprotect(void *addr, size_t len, int prot) {
    return mprotect(addr, len, prot);
}

int sys_getpagesize(void) {
    return getpagesize();
}

int sys_getmemusage(meminfo_t *info) {
    if (info == NULL)
        return -EFAULT;
    
    info->free = pmman.mem_free();
    info->used = pmman.mem_used();

    return 0;
}

int sys_ptsname_r(int fd, char buf[/*..buflen*/], size_t buflen) {
    return ptsname_r(fd, buf, buflen);
}

int sys_mount(const char *src, const char *target, const char *type, unsigned long flags, const void *data) {
    return mount(src, target, type, flags, data);
}

int sys_chmod(const char *pathname __unused, mode_t mode __unused) {
    return -ENOSYS;
}

int sys_chroot(const char *path __unused) {
    return -ENOSYS;
}

int sys_rename(const char *oldpath __unused, const char *newpath __unused) {
    return -ENOSYS;
}

int sys_utimes(const char *filename __unused, const struct timeval times[2] __unused) {
    return -ENOSYS;
}


/* Poll the file descriptors described by the NFDS structures starting at
   FDS.  If TIMEOUT is nonzero and not -1, allow TIMEOUT milliseconds for
   an event to occur; if TIMEOUT is -1, block until an event occurs.
   Returns the number of file descriptors with events, zero if timed out,
   or -1 for errors.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
int sys_poll(struct pollfd fds[] __unused, nfds_t nfds __unused, int timeout __unused) {
    return -ENOSYS; //poll(fds, nfds, timeout);
}

int sys_connect(int sockfd __unused, const struct sockaddr *addr __unused, socklen_t addrlen __unused) {
    return -ENOSYS;
}

int sys_accept(int sockfd __unused, struct sockaddr *restrict addr __unused, socklen_t *restrict addrlen __unused) {
    return -ENOSYS;
}

isize sys_send(int sockfd __unused, const void *buf __unused, usize len __unused, int flags __unused) {
    return -ENOSYS;
}

isize sys_sendto(int sockfd __unused, const void *buf __unused, size_t len __unused, int flags __unused, const struct sockaddr *dest_addr __unused, socklen_t addrlen __unused) {
    return -ENOSYS;
}

isize sys_recv(int sockfd __unused, void *buf __unused, usize len __unused, int flags __unused) {
    return -ENOSYS;
}

int sys_socket(int domain __unused, int type __unused, int protocol __unused) {
    return -ENOSYS;
}

int sys_bind(int sockfd __unused, const struct sockaddr *addr __unused, socklen_t addrlen __unused) {
    return -ENOSYS;
}

int sys_getsockname(int sockfd __unused, struct sockaddr *restrict addr __unused, socklen_t *restrict addrlen __unused) {
    return -ENOSYS;
}

int sys_listen(int sockfd __unused, int backlog __unused) {
    return -ENOSYS;
}

int sys_shutdown(int sockfd __unused, int how __unused) {
    return -ENOSYS;
}

int sys_getpeername(int sockfd __unused, struct sockaddr *restrict addr __unused, socklen_t *restrict addrlen __unused) {
    return -ENOSYS;
}

int sys_getsockopt(int sockfd __unused, int level __unused, int optname __unused, void *optval __unused, socklen_t *restrict optlen __unused) {
    return -ENOSYS;
}

isize sys_recvmsg(int sockfd __unused, struct msghdr *msg __unused, int flags __unused) {
    return -ENOSYS;
}

isize sys_sendmsg(int sockfd __unused, const struct msghdr *msg __unused, int flags __unused) {
    return -ENOSYS;
}

isize sys_recvfrom(int sockfd __unused, void *buf __unused, size_t len __unused, int flags __unused, struct sockaddr *restrict src_addr __unused, socklen_t *restrict addrlen __unused) {
    return -ENOSYS;
}

long sys_ptrace(enum __ptrace_request op __unused, pid_t pid __unused, void *addr __unused, void *data __unused) {
    return -ENOSYS;
}

pid_t sys_vfork(void) {
    return -ENOSYS;
}

int      sys_gettimeofday(struct timeval *restrict tp, void *restrict tzp) {
    return gettimeofday(tp, tzp);
}

int sys_settimeofday(const struct timeval *tv, const struct timezone *tz) {
    return settimeofday(tv, tz);
}

int sys_clock_getres(clockid_t clockid __unused, struct timespec *res __unused) {
    return -ENOSYS;
}

int sys_clock_gettime(clockid_t clockid __unused, struct timespec *tp __unused) {
    return -ENOSYS;
}

int sys_clock_settime(clockid_t clockid __unused, const struct timespec *tp __unused) {
    return -ENOSYS;
}

int sys_mlock(const void *addr __unused, size_t len __unused) {
    return -ENOSYS;
}

int sys_mlock2(const void *addr __unused, size_t len __unused, unsigned int flags __unused) {
    return -ENOSYS;
}

int sys_munlock(const void *addr __unused, size_t len __unused) {
    return -ENOSYS;
}

int sys_mlockall(int flags __unused) {
    return -ENOSYS;
}

int sys_munlockall(void) {
    return -ENOSYS;
}

int sys_madvise(void *addr __unused, size_t length __unused, int advice __unused) {
    return -ENOSYS;
}

void *sys_mremap(void *old_address __unused, size_t old_size __unused, size_t new_size __unused, int flags __unused, ... /* void *new_address */) {
    return NULL;
}

int sys_msync(void *addr __unused, size_t length __unused, int flags __unused) {
    return -ENOSYS;
}

void *sys_sbrk(intptr_t increment) {
    return sbrk(increment);
}

int sys_getrlimit(int resource __unused, void /*struct rlimit*/ *rlim __unused) {
    return -ENOSYS;
}

int sys_setrlimit(int resource __unused, const void /*struct rlimit*/ *rlim __unused) {
    return -ENOSYS;
}

int sys_getrusage(int who __unused, void /*struct rusage*/ *usage __unused) {
    return -ENOSYS;
}

pid_t sys_wait4(pid_t pid __unused, int *wstatus __unused, int options __unused, void /*struct rusage*/ *rusage __unused) {
    return -ENOSYS;
}
