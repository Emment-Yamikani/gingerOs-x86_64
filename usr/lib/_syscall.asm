bits 64

section .text

%macro stub 2
global sys_%2
sys_%2:
    mov rax, %1
    int 0x80
    ret
%endmacro


%define SYS_PUTC            0
%define SYS_CLOSE           1
%define SYS_UNLINK          2
%define SYS_DUP             3
%define SYS_DUP2            4
%define SYS_TRUNCATE        5
%define SYS_FCNTL           6
%define SYS_IOCTL           7
%define SYS_LSEEK           8
%define SYS_READ            9
%define SYS_WRITE           10
%define SYS_OPEN            11
%define SYS_CREATE          12
%define SYS_MKDIRAT         13
%define SYS_READDIR         14
%define SYS_LINKAT          15
%define SYS_MKNODAT         16
%define SYS_SYNC            17
%define SYS_GETATTR         18
%define SYS_SETATTR         19
%define SYS_PARK            20
%define SYS_UNPARK          21
%define SYS_EXIT            22
%define SYS_GETPID          23
%define SYS_GETPPID         24
%define SYS_SLEEP           25

%define SYS_GETTID          26
%define SYS_THREAD_EXIT     27
%define SYS_THREAD_CREATE   28
%define SYS_THREAD_JOIN     29

%define SYS_PAUSE           30
%define SYS_KILL            31
%define SYS_ALARM           32
%define SYS_SIGNAL          33
%define SYS_SIGPROCMASK     34
%define SYS_SIGPENDING      35
%define SYS_SIGACTION       36
%define SYS_PTHREAD_KILL    37
%define SYS_SIGWAIT         38
%define SYS_PTHREAD_SIGMASK 39
%define SYS_THREAD_SELF     40

%define SYS_MMAP            41
%define SYS_UNMAP           42
%define SYS_MPROTECT        43
%define SYS_THREAD_YIELD    44
%define SYS_GETPAGESIZE     45

%define SYS_GETUID          46
%define SYS_GETGID          47
%define SYS_GETEUID         48
%define SYS_GETEGID         49
%define SYS_SETUID          50
%define SYS_SETGID          51
%define SYS_SETEUID         52
%define SYS_SETEGID         53
%define SYS_FORK            54
%define SYS_GETMEMUSAGE     55

%define SYS_GETSID          56
%define SYS_SETSID          57
%define SYS_GETPGRP         58
%define SYS_SETPGRP         60
%define SYS_GETPGID         59
%define SYS_SETPGID         61
%define SYS_WAITPID         62

%define SYS_FSTAT           63
%define SYS_STAT            64
%define SYS_LSTAT           65
%define SYS_FSTATAT         66

%define SYS_WAIT            67
%define SYS_UNAME           68
%define SYS_CHOWN           69
%define SYS_FCHOWN          70
%define SYS_GETTIMEOFDAY    71
%define SYS_UMASK           72
%define SYS_ISATTY          73

%define SYS_GETCWD          74
%define SYS_CHDIR           75
%define SYS_OPENAT          76

stub SYS_PUTC, putc
stub SYS_CLOSE, close
stub SYS_UNLINK, unlink
stub SYS_DUP, dup
stub SYS_DUP2, dup2
stub SYS_TRUNCATE, truncate
stub SYS_FCNTL, fcntl
stub SYS_IOCTL, ioctl
stub SYS_LSEEK, lseek
stub SYS_READ, read
stub SYS_WRITE, write
stub SYS_OPEN, open
stub SYS_OPENAT, openat
stub SYS_CREATE, create
stub SYS_MKDIRAT, mkdirat
stub SYS_READDIR, readdir
stub SYS_LINKAT, linkat
stub SYS_MKNODAT, mknodat
stub SYS_SYNC, sync
stub SYS_GETATTR, getattr
stub SYS_SETATTR, setattr

stub SYS_FSTAT, fstat
stub SYS_STAT, stat
stub SYS_LSTAT, lstat
stub SYS_FSTATAT, fstatat

stub SYS_UNAME, uname
stub SYS_CHOWN, chown
stub SYS_FCHOWN, fchown
stub SYS_GETTIMEOFDAY, gettimeofday
stub SYS_UMASK, umask
stub SYS_ISATTY, isatty

stub SYS_PARK, park
stub SYS_UNPARK, unpark

stub SYS_FORK, fork
stub SYS_WAITPID, waitpid
stub SYS_WAIT, wait
stub SYS_EXIT, exit
stub SYS_GETPID, getpid
stub SYS_GETPPID, getppid

stub SYS_SLEEP, sleep
stub SYS_GETTID, gettid
stub SYS_THREAD_EXIT, thread_exit
stub SYS_THREAD_CREATE, thread_create
stub SYS_THREAD_JOIN, thread_join
stub SYS_THREAD_SELF, thread_self
stub SYS_THREAD_YIELD, thread_yield

stub SYS_PAUSE, pause
stub SYS_KILL, kill
stub SYS_ALARM, alarm
stub SYS_SIGNAL, signal
stub SYS_SIGPROCMASK, sigprocmask
stub SYS_SIGPENDING, sigpending
stub SYS_SIGACTION, sigaction
stub SYS_PTHREAD_KILL, pthread_kill
stub SYS_SIGWAIT, sigwait
stub SYS_PTHREAD_SIGMASK, pthread_sigmask

stub SYS_MMAP, mmap
stub SYS_UNMAP, munmap
stub SYS_MPROTECT, mprotect
stub SYS_GETPAGESIZE, getpagesize
stub SYS_GETMEMUSAGE, getmemusage

stub SYS_GETUID, getuid
stub SYS_GETGID, getgid
stub SYS_GETEUID, geteuid
stub SYS_GETEGID, getegid
stub SYS_SETUID, setuid
stub SYS_SETGID, setgid
stub SYS_SETEUID, seteuid
stub SYS_SETEGID, setegid
stub SYS_GETCWD, getcwd
stub SYS_CHDIR, chdir

stub SYS_GETSID, getsid
stub SYS_SETSID, setsid
stub SYS_GETPGRP, getpgrp
stub SYS_SETPGRP, setpgrp
stub SYS_GETPGID, getpgid
stub SYS_SETPGID, setpgid

