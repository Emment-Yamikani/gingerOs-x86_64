bits 64

section .text

%macro stub 2
global sys_%2
sys_%2:
    mov rax, %1
    int 0x80
    ret
%endmacro


%define SYS_kputc               0

%define SYS_close               1   ; int sys_close(int fd);
%define SYS_unlink              2   ; int sys_unlink(int fd);
%define SYS_dup                 3   ; int sys_dup(int fd);
%define SYS_dup2                4   ; int sys_dup2(int fd1, int fd2);
%define SYS_truncate            5   ; int sys_truncate(int fd, off_t length);
%define SYS_fcntl               6   ; int sys_fcntl(int fd, int cmd, void *argp);
%define SYS_ioctl               7   ; int sys_ioctl(int fd, int req, void *argp);
%define SYS_lseek               8   ; off_t sys_lseek(int fd, off_t off, int whence);
%define SYS_read                9   ; ssize_t sys_read(int fd, void *buf, size_t size);
%define SYS_write               10  ; ssize_t sys_write(int fd, void *buf, size_t size);
%define SYS_open                11  ; int sys_open(const char *pathname, int oflags, mode_t mode);
%define SYS_create              12  ; int sys_create(int fd, const char *filename, mode_t mode);
%define SYS_mkdirat             13  ; int sys_mkdirat(int fd, const char *filename, mode_t mode);
%define SYS_readdir             14  ; ssize_t sys_readdir(int fd, off_t off, void *buf, size_t count);
%define SYS_linkat              15  ; int sys_linkat(int fd, const char *oldname, const char *newname);
%define SYS_mknodat             16  ; int sys_mknodat(int fd, const char *filename, mode_t mode, int devid);
%define SYS_sync                17  ; int sys_sync(int fd);
%define SYS_getattr             18  ; int sys_getattr(int fd, void *attr);
%define SYS_setattr             19  ; int sys_setattr(int fd, void *attr);
%define SYS_mount               20
%define SYS_chmod               21
%define SYS_rename              22
%define SYS_chroot              23
%define SYS_utimes              24

%define SYS_exit                25  ; void sys_exit(int exit_code);
%define SYS_getpid              26  ; pid_t sys_getpid(void);
%define SYS_getppid             27  ; pid_t sys_getppid(void);
%define SYS_fork                28  ; pid_t sys_fork(void);
%define SYS_ptrace              29
%define SYS_vfork               30
%define SYS_wait4               31


%define SYS_park                32  ; int sys_park(void);
%define SYS_unpark              33  ; int sys_unpark(tid_t);
%define SYS_sleep               34  ; long sys_sleep(long seconds);
%define SYS_gettid              35  ; tid_t sys_gettid(void);
%define SYS_thread_exit         36  ; void sys_thread_exit(int exit_code);
%define SYS_thread_create       37  ; int sys_thread_create(tid_t *ptidp, void *attr, thread_entry_t entry, void *arg);
%define SYS_thread_join         38  ; int sys_thread_join(tid_t tid, void **retval);
%define SYS_set_thread_area     39
%define SYS_get_thread_area     40
%define SYS_thread_self         41  ; tid_t sys_thread_self(void)
%define SYS_thread_yield        42  ; void sys_thread_yield(void);

%define SYS_pause               43  ; int sys_pause(void);
%define SYS_raise               44  ; int sys_raise(int signo);
%define SYS_kill                45  ; int sys_kill(pid_t pid, int signo);
%define SYS_alarm               46  ; unsigned sys_alarm(unsigned sec);
%define SYS_signal              47  ; sigfunc_t sys_signal(int signo, sigfunc_t func);
%define SYS_sigprocmask         48  ; int sys_sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);
%define SYS_sigpending          49  ; int sys_sigpending(sigset_t *set);
%define SYS_sigaction           50  ; int sys_sigaction(int signo, const sigaction_t *restrict act, sigaction_t *restrict oact);
%define SYS_pthread_kill        51  ; int sys_pthread_kill(tid_t thread, int signo);
%define SYS_sigwait             52  ; int sys_sigwait(const sigset_t *restrict set, int *restrict signop);
%define SYS_pthread_sigmask     53  ; int sys_pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);
%define SYS_sigaltstack         54
%define SYS_sigreturn           55
%define SYS_sigsuspend          56


%define SYS_mmap                57  ; void *sys_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
%define SYS_munmap              58  ; int sys_munmap(void *addr, size_t len);
%define SYS_mprotect            59  ; int sys_mprotect(void *addr, size_t len, int prot);
%define SYS_mlock               60
%define SYS_mlock2              61
%define SYS_munlock             62
%define SYS_mlockall            63
%define SYS_munlockall          64
%define SYS_msync               65
%define SYS_mremap              66
%define SYS_madvise             67
%define SYS_sbrk                68
%define SYS_getpagesize         69  ; int sys_getpagesize(void);
%define SYS_getmemusage         70  ; void sys_getmemusage(meminfo_t *info);

%define SYS_getuid              71  ; uid_t sys_getuid(void);
%define SYS_getgid              72  ; gid_t sys_getgid(void);
%define SYS_geteuid             73  ; uid_t sys_geteuid(void);
%define SYS_getegid             74  ; gid_t sys_getegid(void);
%define SYS_setuid              75  ; int sys_setuid(uid_t uid);
%define SYS_setgid              76  ; int sys_setgid(gid_t gid);
%define SYS_seteuid             77  ; int sys_seteuid(uid_t euid);
%define SYS_setegid             78  ; int sys_setegid(gid_t egid);

%define SYS_getsid              79  ; pid_t sys_getsid(pid_t pid);
%define SYS_setsid              80  ; pid_t sys_setsid(void);
%define SYS_getpgrp             81  ; pid_t sys_getpgrp(void);
%define SYS_setpgrp             82  ; pid_t sys_setpgrp(void);
%define SYS_getpgid             83  ; pid_t sys_getpgid(pid_t pid);
%define SYS_setpgid             84  ; int   sys_setpgid(pid_t pid, pid_t pgid);
%define SYS_execve              85  ; int sys_execve(const char *pathname, char *const argv[], char *const envp[]);
%define SYS_waitpid             86  ; pid_t sys_waitpid(pid_t __pid, int *__stat_loc, int __options);

%define SYS_pipe                87  ; int sys_pipe(int fds[2]);
%define SYS_fstat               88  ; int sys_fstat(int fildes, struct stat *buf);
%define SYS_stat                89  ; int sys_stat(const char *restrict path, struct stat *restrict buf);
%define SYS_lstat               90  ; int sys_lstat(const char *restrict path, struct stat *restrict buf);
%define SYS_fstatat             91  ; int sys_fstatat(int fd, const char *restrict path, struct stat *restrict buf, int flag);
%define SYS_chown               92  ; int sys_chown(const char *pathname, uid_t uid, gid_t gid);
%define SYS_fchown              93  ; int sys_fchown(int fd, uid_t uid, gid_t gid);
%define SYS_umask               94  ; mode_t sys_umask(mode_t cmask);
%define SYS_isatty              95  ; int sys_isatty(int fd);
%define SYS_mkdir               96  ; int sys_mkdir(const char *filename, mode_t mode);
%define SYS_mknod               97  ; int sys_mknod(const char *filename, mode_t mode, int devid);
%define SYS_getcwd              98  ; int sys_getcwd(char *buf, size_t size);
%define SYS_chdir               99  ; int sys_chdir(const char *path);
%define SYS_openat              100  ; int sys_openat(int fd, const char *pathname, int oflags, mode_t mode)

%define SYS_uname               101  ; int sys_uname(struct utsname *name);

%define SYS_ptsname_r           102  ; int sys_ptsname_r(int fd, char buf[], size_t buflen);

%define SYS_connect             103
%define SYS_accept              104
%define SYS_poll                105
%define SYS_send                106
%define SYS_sendmsg             107
%define SYS_sendto              108
%define SYS_recv                109
%define SYS_recvmsg             110
%define SYS_recvfrom            111
%define SYS_socket              112
%define SYS_bind                113
%define SYS_getsockname         114
%define SYS_listen              115
%define SYS_shutdown            116
%define SYS_getpeername         117
%define SYS_getsockopt          118

%define SYS_gettimeofday        119  ; int sys_gettimeofday(struct timeval *restrict tp, void *restrict tzp);
%define SYS_settimeofday        120
%define SYS_clock_gettime       121
%define SYS_clock_settime       122
%define SYS_clock_getres        123

%define SYS_setrlimit           124
%define SYS_getrlimit           125
%define SYS_getrusage           126


stub SYS_kputc, kputc
stub SYS_close, close
stub SYS_unlink, unlink
stub SYS_dup, dup
stub SYS_dup2, dup2
stub SYS_truncate, truncate
stub SYS_fcntl, fcntl
stub SYS_ioctl, ioctl
stub SYS_lseek, lseek
stub SYS_read, read
stub SYS_write, write
stub SYS_open, open
stub SYS_create, create
stub SYS_mkdirat, mkdirat
stub SYS_readdir, readdir
stub SYS_linkat, linkat
stub SYS_mknodat, mknodat
stub SYS_sync, sync
stub SYS_getattr, getattr
stub SYS_setattr, setattr
stub SYS_mount, mount
stub SYS_chmod, chmod
stub SYS_rename, rename
stub SYS_chroot, chroot
stub SYS_utimes, utimes
stub SYS_exit, exit
stub SYS_getpid, getpid
stub SYS_getppid, getppid
stub SYS_fork, fork
stub SYS_ptrace, ptrace
stub SYS_vfork, vfork
stub SYS_wait4, wait4
stub SYS_park, park
stub SYS_unpark, unpark
stub SYS_sleep, sleep
stub SYS_gettid, gettid
stub SYS_thread_exit, thread_exit
stub SYS_thread_create, thread_create
stub SYS_thread_join, thread_join
stub SYS_set_thread_area, set_thread_area
stub SYS_get_thread_area, get_thread_area
stub SYS_thread_self, thread_self
stub SYS_thread_yield, thread_yield
stub SYS_pause, pause
stub SYS_raise, raise
stub SYS_kill, kill
stub SYS_alarm, alarm
stub SYS_signal, signal
stub SYS_sigprocmask, sigprocmask
stub SYS_sigpending, sigpending
stub SYS_sigaction, sigaction
stub SYS_pthread_kill, pthread_kill
stub SYS_sigwait, sigwait
stub SYS_pthread_sigmask, pthread_sigmask
stub SYS_sigaltstack, sigaltstack
stub SYS_sigreturn, sigreturn
stub SYS_sigsuspend, sigsuspend
stub SYS_mmap, mmap
stub SYS_munmap, munmap
stub SYS_mprotect, mprotect
stub SYS_mlock, mlock
stub SYS_mlock2, mlock2
stub SYS_munlock, munlock
stub SYS_mlockall, mlockall
stub SYS_munlockall, munlockall
stub SYS_msync, msync
stub SYS_mremap, mremap
stub SYS_madvise, madvise
stub SYS_sbrk, sbrk
stub SYS_getpagesize, getpagesize
stub SYS_getmemusage, getmemusage
stub SYS_getuid, getuid
stub SYS_getgid, getgid
stub SYS_geteuid, geteuid
stub SYS_getegid, getegid
stub SYS_setuid, setuid
stub SYS_setgid, setgid
stub SYS_seteuid, seteuid
stub SYS_setegid, setegid
stub SYS_getsid, getsid
stub SYS_setsid, setsid
stub SYS_getpgrp, getpgrp
stub SYS_setpgrp, setpgrp
stub SYS_getpgid, getpgid
stub SYS_setpgid, setpgid
stub SYS_execve, execve
stub SYS_waitpid, waitpid
stub SYS_pipe, pipe
stub SYS_fstat, fstat
stub SYS_stat, stat
stub SYS_lstat, lstat
stub SYS_fstatat, fstatat
stub SYS_chown, chown
stub SYS_fchown, fchown
stub SYS_umask, umask
stub SYS_isatty, isatty
stub SYS_mkdir, mkdir
stub SYS_mknod, mknod
stub SYS_getcwd, getcwd
stub SYS_chdir, chdir
stub SYS_openat, openat
stub SYS_uname, uname
stub SYS_ptsname_r, ptsname_r
stub SYS_connect, connect
stub SYS_accept, accept
stub SYS_poll, poll
stub SYS_send, send
stub SYS_sendmsg, sendmsg
stub SYS_sendto, sendto
stub SYS_recv, recv
stub SYS_recvmsg, recvmsg
stub SYS_recvfrom, recvfrom
stub SYS_socket, socket
stub SYS_bind, bind
stub SYS_getsockname, getsockname
stub SYS_listen, listen
stub SYS_shutdown, shutdown
stub SYS_getpeername, getpeername
stub SYS_getsockopt, getsockopt
stub SYS_gettimeofday, gettimeofday
stub SYS_settimeofday, settimeofday
stub SYS_clock_gettime, clock_gettime
stub SYS_clock_settime, clock_settime
stub SYS_clock_getres, clock_getres
stub SYS_setrlimit, setrlimit
stub SYS_getrlimit, getrlimit
stub SYS_getrusage, getrusage