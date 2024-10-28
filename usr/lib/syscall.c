#include <api.h>

void xputc(int c) {
    sys_putc(c);
}

int close(int fd) {
    return sys_close(fd);
}

int unlink(int fd) {
    return sys_unlink(fd);
}

int dup(int fd) {
    return sys_dup(fd);
}

int dup2(int fd1, int fd2) {
    return sys_dup2(fd1, fd2);
}

int truncate(int fd, off_t length) {
    return sys_truncate(fd, length);
}

int fcntl(int fd, int cmd, void *argp) {
    return sys_fcntl(fd, cmd, argp);
}

int ioctl(int fd, int req, void *argp) {
    return sys_ioctl(fd, req, argp);
}

off_t lseek(int fd, off_t off, int whence) {
    return sys_lseek(fd, off, whence);
}

ssize_t read(int fd, void *buf, size_t size) {
    return sys_read(fd, buf, size);
}

ssize_t write(int fd, void *buf, size_t size) {
    return sys_write(fd, buf, size);
}

int open(const char *pathname, int oflags, mode_t mode) {
    return sys_open(pathname, oflags, mode);
}

int openat(int fd, const char *pathname, int oflags, mode_t mode) {
    return sys_openat(fd, pathname, oflags, mode);
}

int create(const char *filename, mode_t mode) {
    return sys_create(filename, mode);
}

int mkdirat(int fd, const char *filename, mode_t mode) {
    return sys_mkdirat(fd, filename, mode);
}

int linkat(int fd, const char *oldname, const char *newname) {
    return sys_linkat(fd, oldname, newname);
}

int mknodat(int fd, const char *filename, mode_t mode, int devid) {
    return sys_mknodat(fd, filename, mode, devid);
}

int sync(int fd) {
    return sys_sync(fd);
}

int getattr(int fd, void *attr) {
    return sys_getattr(fd, attr);
}

int setattr(int fd, void *attr) {
    return sys_setattr(fd, attr);
}

int fstat(int fildes, struct stat *buf) {
    return sys_fstat(fildes, buf);
}

int stat(const char *restrict path, struct stat *restrict buf) {
    return sys_stat(path, buf);
}

int lstat(const char *restrict path, struct stat *restrict buf) {
    return sys_lstat(path, buf);
}

int fstatat(int fd, const char *restrict path, struct stat *restrict buf, int flag) {
    return sys_fstatat(fd, path, buf, flag);
}

int pipe(int fds[2]) {
    return sys_pipe(fds);
}

int mkdir(const char *filename, mode_t mode) {
    return sys_mkdir(filename, mode);
}

int mknod(const char *filename, mode_t mode, int devid) {
    return sys_mknod(filename, mode, devid);
}


int getcwd(char *buf, size_t size) {
    return sys_getcwd(buf, size);
}

int chdir(const char *path) {
    return sys_chdir(path);
}

pid_t wait(int *stat_loc) {
    return sys_wait(stat_loc);
}

int uname(struct utsname *name) {
    return sys_uname(name);
}

int chown(const char *pathname, uid_t uid, gid_t gid) {
    return sys_chown(pathname, uid, gid);
}

int fchown(int fd, uid_t uid, gid_t gid) {
    return sys_fchown(fd, uid, gid);
}

int gettimeofday(struct timeval *restrict tp, void *restrict tzp) {
    return sys_gettimeofday(tp, tzp);
}

mode_t umask(mode_t cmask) {
    return sys_umask(cmask);
}

int isatty(int fd) {
    return sys_isatty(fd);
}


/** @brief PROTECTION */

uid_t getuid(void) {
    return sys_getuid();
}

uid_t geteuid(void) {
    return sys_geteuid();
}

gid_t getegid(void) {
    return sys_getegid();
}

gid_t getgid(void) {
    return sys_getgid();
}

int setuid(uid_t uid) {
    return sys_setuid(uid);
}

int seteuid(uid_t euid) {
    return sys_seteuid(euid);
}

int setegid(gid_t egid) {
    return sys_setegid(egid);
}

int setgid(gid_t gid) {
    return sys_setgid(gid);
}


int park(void) {
    return sys_park();
}

int unpark(tid_t tid) {
    return sys_unpark(tid);
}


pid_t fork(void) {
    return sys_fork();
}

pid_t waitpid(pid_t __pid, int *__stat_loc, int __options) {
    return sys_waitpid(__pid, __stat_loc, __options);
}

void exit(int exit_code) {
    sys_exit(exit_code);
}

pid_t getpid(void) {
    return sys_getpid();
}

pid_t getppid(void) {
    return sys_getppid();
}

int execve(const char *pathname, char *const argv[],
           char *const envp[]) {
    return sys_execve(pathname, argv, envp);
}

long sleep(long seconds) {
    return sys_sleep(seconds);
}

tid_t gettid(void) {
    return sys_gettid();
}

void thread_exit(int exit_code) {
    sys_thread_exit(exit_code);
}

int thread_create(tid_t *ptidp, void *attr, void *(*entry)(void *arg), void *arg) {
    return sys_thread_create(ptidp, attr, entry, arg);
}

int thread_join(tid_t tid, void **retval) {
    return sys_thread_join(tid, retval);
}

tid_t thread_self(void) {
    return sys_thread_self();
}

void thread_yield(void) {
    sys_thread_yield();
}

/** @brief SIGNALS */

int pause(void) {
    return sys_pause();
}

int raise(int signo) {
    return sys_raise(signo);
}

int kill(pid_t pid, int signo) {
    return sys_kill(pid, signo);
}

unsigned  long alarm(unsigned long sec) {
    return sys_alarm(sec);
}

sigfunc_t signal(int signo, sigfunc_t func) {
    return sys_signal(signo, func);
}

int sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset) {
    return sys_sigprocmask(how, set, oset);
}

int sigpending(sigset_t *set) {
    return sys_sigpending(set);
}

int sigaction(int signo, const sigaction_t *restrict act, sigaction_t *restrict oact) {
    return sys_sigaction(signo, act, oact);
}

int pthread_kill(tid_t thread, int signo) {
    return sys_pthread_kill(thread, signo);
}

int sigwait(const sigset_t *restrict set, int *restrict signop) {
    return sys_sigwait(set, signop);
}

int pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset) {
    return sys_pthread_sigmask(how, set, oset);
}

/** @brief MEMORY MANAGEMENT */

void *mmap(void *addr, size_t len, int prot, int flags, int fd, long off) {
    return sys_mmap(addr, len, prot, flags, fd, off);
}

int munmap(void *addr, size_t len) {
    return sys_munmap(addr, len);
}

int mprotect(void *addr, size_t len, int prot) {
    return sys_mprotect(addr, len, prot);
}

int getpagesize(void) {
    return sys_getpagesize();
}

int getmemusage(meminfo_t *info) {
    return sys_getmemusage(info);
}

/** @brief  PROCESS(JOB) RELATIONSHIPS. */

pid_t getsid(pid_t pid) {
    return sys_getsid(pid);
}

pid_t setsid(void) {
    return sys_setsid();
}

pid_t getpgrp(void) {
    return sys_getpgrp();
}

pid_t setpgrp(void) {
    return sys_setpgrp();
}

pid_t getpgid(pid_t pid) {
    return sys_getpgid(pid);
}

int setpgid(pid_t pid, pid_t pgid) {
    return sys_setpgid(pid, pgid);
}