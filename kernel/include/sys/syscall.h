#pragma once

#include <arch/x86_64/context.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <sys/thread.h>
#include <mm/mm_zone.h>
#include <fs/stat.h>

void do_syscall(tf_t *tf);

#define SYS_PUTC                0

#define SYS_CLOSE               1   // sys_close(int fd);
#define SYS_UNLINK              2   // sys_unlink(int fd);
#define SYS_DUP                 3   // sys_dup(int fd);
#define SYS_DUP2                4   // sys_dup2(int fd1, int fd2);
#define SYS_TRUNCATE            5   // sys_truncate(int fd, off_t length);
#define SYS_FCNTL               6   // sys_fcntl(int fd, int cmd, void *argp);
#define SYS_IOCTL               7   // sys_ioctl(int fd, int req, void *argp);
#define SYS_LSEEK               8   // sys_lseek(int fd, off_t off, int whence);
#define SYS_READ                9   // sys_read(int fd, void *buf, size_t size);
#define SYS_WRITE               10  // sys_write(int fd, void *buf, size_t size);
#define SYS_OPEN                11  // sys_open(const char *pathname, int oflags, mode_t mode);
#define SYS_CREATE              12  // sys_create(int fd, const char *filename, mode_t mode);
#define SYS_MKDIRAT             13  // sys_mkdirat(int fd, const char *filename, mode_t mode);
#define SYS_READDIR             14  // sys_readdir(int fd, off_t off, void *buf, size_t count);
#define SYS_LINKAT              15  // sys_linkat(int fd, const char *oldname, const char *newname);
#define SYS_MKNODAT             16  // sys_mknodat(int fd, const char *filename, mode_t mode, int devid);
#define SYS_SYNC                17  // sys_sync(int fd);
#define SYS_GETATTR             18  // sys_getattr(int fd, void *attr);
#define SYS_SETATTR             19  // sys_setattr(int fd, void *attr);

#define SYS_PARK                20  // int sys_park(void);
#define SYS_UNPARK              21  // int sys_unpark(tid_t);
#define SYS_EXIT                22  // void sys_exit(int exit_code);
#define SYS_GETPID              23  // pid_t sys_getpid(void);
#define SYS_GETPPID             24  // pid_t sys_getppid(void);


#define SYS_SLEEP               25  // long sys_sleep(long seconds);
#define SYS_GETTID              26  // tid_t sys_gettid(void);
#define SYS_THREAD_EXIT         27  // void sys_thread_exit(int exit_code);
#define SYS_THREAD_CREATE       28  // int sys_thread_create(tid_t *ptidp, void *attr, thread_entry_t entry, void *arg);
#define SYS_THREAD_JOIN         29  // int sys_thread_join(tid_t tid, void **retval);

#define SYS_PAUSE               30  // int sys_pause(void);
#define SYS_KILL                31  // int sys_kill(pid_t pid, int signo);
#define SYS_ALARM               32  // unsigned long sys_alarm(unsigned long sec);
#define SYS_SIGNAL              33  // sigfunc_t sys_signal(int signo, sigfunc_t func);
#define SYS_SIGPROCMASK         34  // int sys_sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);
#define SYS_SIGPENDING          35  // int sys_sigpending(sigset_t *set);
#define SYS_SIGACTION           36  // int sys_sigaction(int signo, const sigaction_t *restrict act, sigaction_t *restrict oact);
#define SYS_PTHREAD_KILL        37  // int sys_pthread_kill(tid_t thread, int signo);
#define SYS_SIGWAIT             38  // int sys_sigwait(const sigset_t *restrict set, int *restrict signop);
#define SYS_PTHREAD_SIGMASK     39  // int sys_pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);
#define SYS_THREAD_SELF         40  // tid_t sys_thread_self(void)
#define SYS_MMAP                41  // void *sys_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
#define SYS_UNMAP               42  // int sys_munmap(void *addr, size_t len);
#define SYS_MPROTECT            43  // int sys_mprotect(void *addr, size_t len, int prot);
#define SYS_THREAD_YIELD        44  // void sys_thread_yield(void);
#define SYS_GETPAGESIZE         45  // int sys_getpagesize(void);
#define SYS_GETUID              46  // uid_t sys_getuid(void);
#define SYS_GETGID              47  // gid_t sys_getgid(void);
#define SYS_GETEUID             48  // uid_t sys_geteuid(void);
#define SYS_GETEGID             49  // gid_t sys_getegid(void);
#define SYS_SETUID              50  // int sys_setuid(uid_t uid);
#define SYS_SETGID              51  // int sys_setgid(gid_t gid);
#define SYS_SETEUID             52  // int sys_seteuid(uid_t euid);
#define SYS_SETEGID             53  // int sys_setegid(gid_t egid);

#define SYS_FORK                54  // pid_t sys_fork(void);
#define SYS_GETMEMUSAGE         55  // void sys_getmemusage(meminfo_t *info);

#define SYS_GETSID              56  // pid_t sys_getsid(pid_t pid);
#define SYS_SETSID              57  // pid_t sys_setsid(void);
#define SYS_GETPGRP             58  // pid_t sys_getpgrp(void);
#define SYS_SETPGRP             60  // pid_t sys_setpgrp(void);
#define SYS_GETPGID             59  // pid_t sys_getpgid(pid_t pid);
#define SYS_SETPGID             61  // int   sys_setpgid(pid_t pid, pid_t pgid);
#define SYS_WAITPID             62  // pid_t sys_waitpid(pid_t __pid, int *__stat_loc, int __options);

#define SYS_FSTAT               63  // int sys_fstat(int fildes, struct stat *buf);
#define SYS_STAT                64  // int sys_stat(const char *restrict path, struct stat *restrict buf);
#define SYS_LSTAT               65  // int sys_lstat(const char *restrict path, struct stat *restrict buf);
#define SYS_FSTATAT             66  // int sys_fstatat(int fd, const char *restrict path, struct stat *restrict buf, int flag);

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

extern int      sys_fstat(int fildes, struct stat *buf);
extern int      sys_stat(const char *restrict path, struct stat *restrict buf);
extern int      sys_lstat(const char *restrict path, struct stat *restrict buf);
extern int      sys_fstatat(int fd, const char *restrict path, struct stat *restrict buf, int flag);

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

extern pid_t   sys_getsid(pid_t pid);
extern pid_t   sys_setsid(void);
extern pid_t   sys_getpgrp(void);
extern pid_t   sys_setpgrp(void);
extern pid_t   sys_getpgid(pid_t pid);
extern int     sys_setpgid(pid_t pid, pid_t pgid);

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
extern void     sys_thread_yield(void);

/** @brief MEMORY MANAGEMENT */

extern int sys_getpagesize(void);
extern int sys_munmap(void *addr, size_t len);
extern int sys_mprotect(void *addr, size_t len, int prot);
extern void *sys_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
extern int sys_getmemusage(meminfo_t *info);