#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>
#include <lib/types.h>
#include <ds/queue.h>
#include <sync/spinlock.h>

#define SIGABRT     1  // abnormal termination (abort)         | terminate+core.
#define SIGALRM     2  // timer expired (alarm)                | terminate.
#define SIGBUS      3  // hardware fault                       | terminate+core.
#define SIGCANCEL   4  // threads library internal use         | ignore.
#define SIGCHLD     5  // change in status of child            | ignore.
#define SIGCONT     6  // continue stopped process             | continue/ignore.
#define SIGEMT      7  // hardware fault                       | terminate+core.
#define SIGFPE      8  // arithmetic exception                 | terminate+core.
#define SIGHUP      9  // hangup                               | terminate.
#define SIGILL      10 // illegal instruction                  | terminate+core.
#define SIGINT      11 // terminal interrupt character         | terminate.
#define SIGIO       12 // asynchronous I/O                     | terminate/ignore.
#define SIGIOT      13 // hardware fault                       | terminate+core.
#define SIGKILL     14 // termination                          | terminate.
#define SIGPIPE     15 // write to pipe with no readers        | terminate.
#define SIGPROF     16 // profiling time alarm (setitimer)     | terminate.
#define SIGQUIT     17 // terminal quit character              | terminate+core.
#define SIGSEGV     18 // invalid memory reference             | terminate+core.
#define SIGSTOP     19 // stop                                 | stop process.
#define SIGSYS      20 // invalid system call                  | terminate+core.
#define SIGTERM     21 // termination                          | terminate.
#define SIGTRAP     22 // hardware fault                       | terminate+core.
#define SIGTSTP     23 // terminal stop character              | stop process.
#define SIGTTIN     24 // background read from control tty     | stop process.
#define SIGTTOU     25 // background write to control tty      | stop process.
#define SIGURG      26 // urgent condition (sockets)           | ignore.
#define SIGUSR1     27 // user-defined signal                  | terminate.
#define SIGUSR2     28 // user-defined signal                  | terminate.
#define SIGVTALRM   29 // virtual time alarm (setitimer)       | terminate.
#define SIGWINCH    30 // terminal window size change          | ignore.
#define SIGXCPU     31 // CPU limit exceeded (setrlimit)       | terminate or terminate+core.
#define SIGXFSZ     32 // file size limit exceeded (setrlimit) | terminate or terminate+core.

#define NSIG        32

typedef void        (*sigfunc_t)();


#define SIG_ERR     ((sigfunc_t)-1)  // error setting signal disposition.
#define SIG_DFL     ((sigfunc_t)0)   // default action taken.
#define SIG_IGN     ((sigfunc_t)1)   // ignore this signal.

#define SIG_IGNORE      (1)
#define SIG_ABRT        (2)
#define SIG_TERM        (3)
#define SIG_TERM_CORE   (4)
#define SIG_STOP        (5)
#define SIG_CONT        (6)

extern const char *signal_str[];

extern const int sig_defaults[];

typedef unsigned long sigset_t;

#define SIGBAD(signo) ({ ((signo) < 1 || (signo) > NSIG); })

#define sigemptyset(set) ({   \
    int err = 0;              \
    if ((set) == NULL)        \
        err = -EINVAL;        \
    else                      \
        *(set) = (sigset_t)0; \
    err;                      \
})

#define sigfillset(set) ({     \
    int err = 0;               \
    if ((set) == NULL)         \
        err = -EINVAL;         \
    else                       \
        *(set) = ~(sigset_t)0; \
    err;                       \
})

#define sigaddset(set, signo) ({                \
    int err = 0;                                \
    if ((set) == NULL || SIGBAD(signo))         \
        err = -EINVAL;                          \
    else                                        \
        *(set) |= (sigset_t)(1 << ((signo)-1)); \
    err;                                        \
})

#define sigdelset(set, signo) ({                 \
    int err = 0;                                 \
    if ((set) == NULL || SIGBAD(signo))          \
        err = -EINVAL;                           \
    else                                         \
        *(set) &= ~(sigset_t)(1 << ((signo)-1)); \
    err;                                         \
})

#define sigismember(set, signo) ({                            \
    int err = 0;                                              \
    if ((set) == NULL || SIGBAD(signo))                       \
        err = -EINVAL;                                        \
    else                                                      \
        err = ((*(set) & (sigset_t)(1 << ((signo)-1))) != 0); \
    err;                                                      \
})

extern int pause(void);
extern int raise(int signo);
extern int kill(pid_t pid, int signo);
extern unsigned long alarm(unsigned long sec);
extern sigfunc_t signal(int signo, sigfunc_t func);

#define SIG_BLOCK   (1)
#define SIG_UNBLOCK (2)
#define SIG_SETMASK (3)

extern int sigpending(sigset_t *set);
extern int sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);

/*
If signo is SIGCHLD, do not generate this signal
when a child process stops (job control). This
signal is still generated, of course, when a child
terminates (but see the SA_NOCLDWAIT option
below).
*/
#define SA_NOCLDSTOP    BS(0)

/*
If signo is SIGCHLD, this option prevents the
system from creating zombie processes when
children of the calling process terminate. If it
subsequently calls wait, the calling process
blocks until all its child processes have
terminated and then returns −1 with errno set
to ECHILD.
*/
#define SA_NOCLDWAIT    BS(1)

/*
When this signal is caught, the signal is not
automatically blocked by the system while the
signal-catching function executes (unless the
signal is also included in sa_mask). Note that
this type of operation corresponds to the earlier
unreliable signals.
*/
#define SA_NODEFER      BS(2)

/*
If an alternative stack has been declared with
sigaltstack(2), this signal is delivered to the
process on the alternative stack.
*/
#define SA_ONSTACK      BS(3)

/*
The disposition for this signal is reset to
SIG_DFL, and the SA_SIGINFO flag is cleared
on entry to the signal-catching function. Note
that this type of operation corresponds to the
earlier unreliable signals. The disposition for
the two signals SIGILL and SIGTRAP can’t be
reset automatically, however. Setting this flag
can optionally cause sigaction to behave as if
SA_NODEFER is also set.
*/
#define SA_RESETHAND    BS(4)

/*
System calls interrupted by this signal are
automatically restarted.
*/
#define SA_RESTART      BS(5)

/*
This option provides additional information to a
signal handler: a pointer to a siginfo structure
and a pointer to an identifier for the process
context.
*/
#define SA_SIGINFO      BS(6)

union sigval {
    int             sigval_int;     /* Integer value */
    void            *sigval_ptr;    /* Pointer value */
};

typedef struct {
    int             si_signo;   /* Signal number */
    int             si_code;    /* Signal code */
    pid_t           si_pid;     /* Sending process ID */
    uid_t           si_uid;     /* Real user ID of sending process */
    void            *si_addr;   /* Address of faulting instruction */
    int             si_status;  /* Exit value or signal */
    union sigval    si_value;   /* Signal value */
} siginfo_t;

typedef struct __uc_stack_t {
    void    *ss_sp;     /* stack base or pointer */
    size_t  ss_size;    /* stack size */
    i32     ss_flags;   /* flags */
} __packed uc_stack_t;

typedef struct {
    void        (*sa_handler)(int); /* addr of signal handler, or SIG_IGN, or SIG_DFL */
    sigset_t    sa_mask;            /* additional signals to block */
    int         sa_flags;           /* signal options, Figure 10.16 */
    /* alternate handler */
    void        (*sa_sigaction)(int, siginfo_t *, void *ucontext);
} sigaction_t;

struct queue;
typedef struct __sig_desc_t {
    sigset_t    sig_mask;           // signal set mask.
    queue_t     sig_queue[NSIG];    // queues of siginfo_t * pointers for each signal instance.
    sigaction_t sig_action[NSIG];   // signal action entry for each signal.
    spinlock_t  sig_lock;           // spinlock to protect this struct.
} sig_desc_t;

#define sigdesc_assert(desc)            ({ assert(desc, "No signal description."); })
#define sigdesc_lock(desc)              ({ sigdesc_assert(desc); spin_lock(&(desc)->sig_lock); })
#define sigdesc_unlock(desc)            ({ sigdesc_assert(desc); spin_unlock(&(desc)->sig_lock); })
#define sigdesc_islocked(desc)          ({ sigdesc_assert(desc); spin_islocked(&(desc)->sig_lock); })
#define sigdesc_assert_locked(desc)     ({ sigdesc_assert(desc); spin_assert_locked(&(desc)->sig_lock); })

extern void sig_desc_free(sig_desc_t *desc);
extern int sig_desc_alloc(sig_desc_t **pdesc);
extern void sigdequeue_pending(queue_t *queue, siginfo_t **ret);
extern int sigenqueue_pending(queue_t *sigqueue, siginfo_t *info);

extern int pthread_kill(tid_t thread, int signo);
extern int sigwait(const sigset_t *restrict set, int *restrict signop);
extern int pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);
extern int sigaction(int signo, const sigaction_t *restrict act, sigaction_t *restrict oact);

extern int signal_dispatch(void);