#pragma once

#include <stdint.h>
#include <stddef.h>
#include <types.h>
#include <sys/_signal.h>
#include <ginger/ginger.h>

extern void     sys_putc(int c);
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
extern int      sys_create(int fd, const char *filename, mode_t mode);
extern int      sys_mkdirat(int fd, const char *filename, mode_t mode);
extern ssize_t  sys_readdir(int fd, off_t off, void *buf, size_t count);
extern int      sys_linkat(int fd, const char *oldname, const char *newname);
extern int      sys_mknodat(int fd, const char *filename, mode_t mode, int devid);
extern int      sys_sync(int fd);
extern int      sys_getattr(int fd, void *attr);
extern int      sys_setattr(int fd, void *attr);

/** @brief PROTECTION */

extern uid_t    sys_getuid(void);
extern uid_t    sys_geteuid(void);
extern gid_t    sys_getegid(void);
extern gid_t    sys_getgid(void);
extern int      sys_setuid(uid_t uid);
extern int      sys_seteuid(uid_t euid);
extern int      sys_setegid(gid_t egid);
extern int      sys_setgid(gid_t gid);

extern int      sys_setsid(pid_t pid);
extern pid_t    sys_getsid(void);


extern int      sys_park(void);
extern int      sys_unpark(tid_t);

extern void     sys_exit(int exit_code);
extern pid_t    sys_fork(void);
extern pid_t    sys_getpid(void);
extern pid_t    sys_getppid(void);

extern long     sys_sleep(long seconds);
extern tid_t    sys_gettid(void);
extern void     sys_thread_exit(int exit_code);
extern int      sys_thread_create(tid_t *ptidp, void *attr, void *(*entry)(void *arg), void *arg);
extern int      sys_thread_join(tid_t tid, void **retval);
extern tid_t    sys_thread_self(void);
extern void     sys_thread_yield();

/** @brief SIGNALS */

extern int      sys_pause(void);
extern int      sys_kill(pid_t pid, int signo);
extern unsigned long sys_alarm(unsigned long sec);
extern sigfunc_t sys_signal(int signo, sigfunc_t func);
extern int      sys_sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);
extern int      sys_sigpending(sigset_t *set);
extern int      sys_sigaction(int signo, const sigaction_t *restrict act, sigaction_t *restrict oact);
extern int      sys_pthread_kill(tid_t thread, int signo);
extern int      sys_sigwait(const sigset_t *restrict set, int *restrict signop);
extern int      sys_pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);

/** @brief MEMORY MANAGEMENT */

void *sys_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
int sys_munmap(void *addr, size_t len);
int sys_mprotect(void *addr, size_t len, int prot);
int sys_getpagesize(void);
int sys_getmemusage(meminfo_t *info);