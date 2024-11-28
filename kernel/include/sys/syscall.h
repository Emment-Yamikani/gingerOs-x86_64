#pragma once

#include <arch/ucontext.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <sys/thread.h>
#include <mm/zone.h>
#include <fs/stat.h>
#include <sys/_time.h>
#include <sys/_utsname.h>
#include <sys/syscall_nums.h>
#include <sys/_socket.h>
#include <sys/_ptrace.h>
#include <sys/poll.h>

void do_syscall(ucontext_t *uctx);

extern void     sys_kputc(int c);

extern int      sys_close(int fd);
extern int      sys_unlink(int fd);
extern int      sys_dup(int fd);
extern int      sys_dup2(int fd1, int fd2);
extern int      sys_truncate(int fd, off_t length);
extern int      sys_fcntl(int fd, int cmd, void *argp);
extern int      sys_ioctl(int fd, int req, void *argp);
extern off_t    sys_lseek(int fd, off_t off, int whence);
extern ssize_t  sys_read(int fd, void *buf, size_t size);
extern ssize_t  sys_write(int fd, void *buf, size_t size);
extern int      sys_open(const char *pathname, int oflags, mode_t mode);
extern int      sys_openat(int fd, const char *pathname, int oflags, mode_t mode);
extern int      sys_create(const char *filename, mode_t mode);
extern int      sys_mkdirat(int fd, const char *filename, mode_t mode);
extern ssize_t  sys_readdir(int fd, off_t off, void *buf, size_t count);
extern int      sys_linkat(int fd, const char *oldname, const char *newname);
extern int      sys_mknodat(int fd, const char *filename, mode_t mode, int devid);
extern int      sys_mkdir(const char *filename, mode_t mode);
extern int      sys_mknod(const char *filename, mode_t mode, int devid);

extern int      sys_sync(int fd);
extern int      sys_pipe(int fds[2]);
extern int      sys_getattr(int fd, void *attr);
extern int      sys_setattr(int fd, void *attr);

extern int      sys_fstat(int fildes, struct stat *buf);
extern int      sys_stat(const char *restrict path, struct stat *restrict buf);
extern int      sys_lstat(const char *restrict path, struct stat *restrict buf);
extern int      sys_fstatat(int fd, const char *restrict path, struct stat *restrict buf, int flag);

extern int      sys_uname(struct utsname *name);
extern int      sys_chown(const char *pathname, uid_t uid, gid_t gid);
extern int      sys_fchown(int fd, uid_t uid, gid_t gid);
extern mode_t   sys_umask(mode_t cmask);
extern int      sys_mount(const char *source, const char *target, const char *type, unsigned long flags, const void *data);
extern int      sys_chmod(const char *pathname, mode_t mode);
extern int      sys_chroot(const char *path);
extern int      sys_rename(const char *oldpath, const char *newpath);
extern int      sys_utimes(const char *filename, const struct timeval times[2]);

/** @brief PROTECTION */

extern uid_t    sys_getuid(void);
extern uid_t    sys_geteuid(void);
extern gid_t    sys_getegid(void);
extern gid_t    sys_getgid(void);
extern int      sys_setuid(uid_t uid);
extern int      sys_seteuid(uid_t euid);
extern int      sys_setegid(gid_t egid);
extern int      sys_setgid(gid_t gid);

extern int      sys_park(void);
extern int      sys_unpark(tid_t);
extern pid_t    sys_fork(void);
extern void     sys_exit(int exit_code);
extern long     sys_sleep(long seconds);
extern pid_t    sys_waitpid(pid_t __pid, int *__stat_loc, int __options);
extern int      sys_execve(const char *pathname, char *const argv[],
                  char *const envp[]);

extern tid_t    sys_gettid(void);
extern void     sys_thread_exit(int exit_code);
extern int      sys_thread_create(tid_t *ptidp, void *attr, thread_entry_t entry, void *arg);
extern int      sys_thread_join(tid_t tid, void **retval);
extern tid_t    sys_thread_self(void);

extern void     exit(int exit_code);
extern pid_t    getpid(void);
extern pid_t    getppid(void);

extern pid_t    sys_getpid(void);
extern pid_t    sys_getppid(void);

extern pid_t    sys_getsid(pid_t pid);
extern pid_t    sys_setsid(void);
extern pid_t    sys_getpgrp(void);
extern pid_t    sys_setpgrp(void);
extern pid_t    sys_getpgid(pid_t pid);
extern int      sys_setpgid(pid_t pid, pid_t pgid);

extern int      sys_getcwd(char *buf, size_t size);
extern int      sys_chdir(const char *path);

/** @brief SIGNALS */

extern int      sys_pause(void);
extern int      sys_raise(int signo);
extern int      sys_kill(pid_t pid, int signo);
extern unsigned sys_alarm(unsigned sec);
extern sigfunc_t sys_signal(int signo, sigfunc_t func);
extern int      sys_sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);
extern int      sys_sigpending(sigset_t *set);
extern int      sys_sigaction(int signo, const sigaction_t *restrict act, sigaction_t *restrict oact);
extern int      sys_pthread_kill(tid_t thread, int signo);
extern int      sys_sigwait(const sigset_t *restrict set, int *restrict signop);
extern int      sys_pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);
extern void     sys_thread_yield(void);

/** @brief MEMORY MANAGEMENT */

extern int      sys_munmap(void *addr, size_t len);
extern int      sys_getpagesize(void);
extern int      sys_mprotect(void *addr, size_t len, int prot);
extern void     *sys_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
extern int      sys_getmemusage(meminfo_t *info);

extern int      sys_isatty(int fd);
extern int      sys_ptsname_r(int fd, char buf[], size_t buflen);

extern int      sys_poll(struct pollfd fds[] __unused, nfds_t nfds __unused, int timeout __unused);

extern int      sys_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern int      sys_accept(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);
extern isize    sys_send(int sockfd, const void *buf, usize len, int flags);
extern isize    sys_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
extern isize    sys_recv(int sockfd, void *buf, usize len, int flags);
extern int      sys_socket(int domain, int type, int protocol);
extern int      sys_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern int      sys_getsockname(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);
extern int      sys_listen(int sockfd, int backlog);
extern int      sys_shutdown(int sockfd, int how);
extern int      sys_getpeername(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);
extern int      sys_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *restrict optlen);
extern isize    sys_recvmsg(int sockfd, struct msghdr *msg, int flags);
extern isize    sys_sendmsg(int sockfd, const struct msghdr *msg, int flags);
extern isize    sys_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *restrict src_addr, socklen_t *restrict addrlen);
extern long     sys_ptrace(enum __ptrace_request op, pid_t pid, void *addr, void *data);
extern pid_t    sys_vfork(void);
extern int      sys_set_thread_area(void *addr);
extern int      sys_get_thread_area(void *addr);
extern void     sys_sigreturn();

extern int      sys_gettimeofday(struct timeval *restrict tp, void *restrict tzp);
extern int      sys_settimeofday(const struct timeval *tv, const struct timezone *tz);
extern int      sys_clock_getres(clockid_t clockid, struct timespec *res);
extern int      sys_clock_gettime(clockid_t clockid, struct timespec *tp);
extern int      sys_clock_settime(clockid_t clockid, const struct timespec *tp);

extern int      sys_mlock(const void *addr, size_t len);
extern int      sys_mlock2(const void *addr, size_t len, unsigned int flags);
extern int      sys_munlock(const void *addr, size_t len);
extern int      sys_mlockall(int flags);
extern int      sys_munlockall(void);
extern int      sys_madvise(void *addr, size_t length, int advice);
extern void     *sys_mremap(void *old_address, size_t old_size, size_t new_size, int flags, ... /* void *new_address */);
extern int      sys_msync(void *addr, size_t length, int flags);
extern void     *sys_sbrk(intptr_t increment);

extern int      sys_getrlimit(int resource, void /*struct rlimit*/ *rlim);
extern int      sys_setrlimit(int resource, const void /*struct rlimit*/ *rlim);
extern int      sys_getrusage(int who, void /*struct rusage*/ *usage);

extern pid_t    sys_wait4(pid_t pid, int *wstatus, int options, void /*struct rusage*/ *rusage);
extern int      sys_sigsuspend(const sigset_t *mask);
extern int      sys_sigaltstack(const stack_t *restrict ss, stack_t *restrict old_ss);