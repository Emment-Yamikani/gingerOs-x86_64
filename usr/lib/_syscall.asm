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
stub SYS_CREATE, create
stub SYS_MKDIRAT, mkdirat
stub SYS_READDIR, readdir
stub SYS_LINKAT, linkat
stub SYS_MKNODAT, mknodat
stub SYS_SYNC, sync
stub SYS_GETATTR, getattr
stub SYS_SETATTR, setattr
stub SYS_PARK, park
stub SYS_UNPARK, unpark
stub SYS_EXIT, exit
stub SYS_GETPID, getpid
stub SYS_GETPPID, getppid
stub SYS_SLEEP, sleep
stub SYS_GETTID, gettid
stub SYS_THREAD_EXIT, thread_exit
stub SYS_THREAD_CREATE, thread_create
stub SYS_THREAD_JOIN, thread_join
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
stub SYS_THREAD_SELF, thread_self