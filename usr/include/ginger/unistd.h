#pragma once

#include <api.h>

void xputc(int c);
int close(int fd);
int unlink(int fd);
int dup(int fd);
int dup2(int fd1, int fd2);
int truncate(int fd, off_t length);
int fcntl(int fd, int cmd, void *argp);
int ioctl(int fd, int req, void *argp);
off_t lseek(int fd, off_t off, int whence);
ssize_t read(int fd, void *buf, size_t size);
ssize_t write(int fd, void *buf, size_t size);
int open(const char *pathname, int oflags, mode_t mode);
int openat(int fd, const char *pathname, int oflags, mode_t mode);
int create(const char *filename, mode_t mode);
int mkdirat(int fd, const char *filename, mode_t mode);

int mkdir(const char *filename, mode_t mode);
int mknod(const char *filename, mode_t mode, int devid);
int pipe(int fds[2]);

int linkat(int fd, const char *oldname, const char *newname);
int mknodat(int fd, const char *filename, mode_t mode, int devid);
int sync(int fd);
int getattr(int fd, void *attr);
int setattr(int fd, void *attr);
int fstat(int fildes, struct stat *buf);
int stat(const char *restrict path, struct stat *restrict buf);
int lstat(const char *restrict path, struct stat *restrict buf);
int fstatat(int fd, const char *restrict path, struct stat *restrict buf, int flag);
int getcwd(char *buf, size_t size);
int chdir(const char *path);
pid_t wait(int *stat_loc);
int uname(struct utsname *name);
int chown(const char *pathname, uid_t uid, gid_t gid);
int fchown(int fd, uid_t uid, gid_t gid);
int gettimeofday(struct timeval *restrict tp, void *restrict tzp);
mode_t umask(mode_t cmask);
int isatty(int fd);
uid_t getuid(void);
uid_t geteuid(void);
gid_t getegid(void);
gid_t getgid(void);
int setuid(uid_t uid);
int seteuid(uid_t euid);
int setegid(gid_t egid);
int setgid(gid_t gid);
int park(void);
int unpark(tid_t tid);


pid_t fork(void);
pid_t waitpid(pid_t __pid, int *__stat_loc, int __options);
void exit(int exit_code);
pid_t getpid(void);
pid_t getppid(void);
int execve(const char *pathname, char *const argv[],
           char *const envp[]);

long sleep(long seconds);
tid_t gettid(void);
void thread_exit(int exit_code);
int thread_create(tid_t *ptidp, void *attr, void *(*entry)(void *arg), void *arg);
int thread_join(tid_t tid, void **retval);
tid_t thread_self(void);
void thread_yield(void);

int pause(void);
int raise(int signo);
int kill(pid_t pid, int signo);
unsigned  long alarm(unsigned long sec);
sigfunc_t signal(int signo, sigfunc_t func);
int sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);
int sigpending(sigset_t *set);
int sigaction(int signo, const sigaction_t *restrict act, sigaction_t *restrict oact);
int pthread_kill(tid_t thread, int signo);
int sigwait(const sigset_t *restrict set, int *restrict signop);
int pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);

int getmemusage(meminfo_t *info);

pid_t getsid(pid_t pid);
pid_t setsid(void);
pid_t getpgrp(void);
pid_t setpgrp(void);
pid_t getpgid(pid_t pid);
int setpgid(pid_t pid, pid_t pgid);