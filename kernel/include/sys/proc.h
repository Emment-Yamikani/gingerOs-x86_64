#pragma once

#include <bits/errno.h>
#include <ds/queue.h>
#include <fs/cred.h>
#include <lib/stdint.h>
#include <lib/stddef.h>
#include <lib/types.h>
#include <mm/mmap.h>
#include <sync/cond.h>
#include <sync/spinlock.h>
#include <sys/thread.h>

struct session;
struct pgroup;

typedef enum status_t {
    P_EMBROY,
    P_RUNNING,
    P_STOPPED,
    P_CONTINUED,
    P_TERMINATED,
    P_ZOMBIE,
} status_t;

typedef struct proc {
    pid_t           pid;            // process' ID.
    pid_t           pgroup;         // process' group
    pid_t           session;        // process' session
    struct proc     *parent;        // process' parent.
    
    status_t        state;          // process' status.
    unsigned long   flags;          // process' ho   
    long            exit_code;      // process' exit status
    thread_entry_t  entry;          // process' entry point.
    long            refcnt;         // process' reference count.

    char            *name;          // process' name.
    mmap_t          *mmap;          // process' memory map(virtual address space).
    
    file_ctx_t      *fctx;
    cred_t          *cred;
    sig_desc_t      *sigdesc;
    queue_t         *threads;

    thread_t        *main_thread;
    cond_t          child_event;    // process' child wait-event condition.
    queue_t         children;       // process' children queue.

    spinlock_t      lock;           // lock to protect this structure.
} proc_t;

#define PROC_USER               BS(0)   // process is a user process.
#define PROC_EXECED             BS(1)   // process has executed exec().
#define PROC_KILLED             BS(2)   // process killed.
#define PROC_ORPHANED           BS(3)   // process was orphaned by parent.
#define PROC_REAP               BS(5)   // process struct marked for reaping.

#define NPROC                   (32786)
#define curproc                 ({ current ? current->t_owner : (proc_t *)NULL; })                //

extern queue_t *procQ;

/// INIT process of the system.
extern proc_t *initproc;

#define proc_assert(proc)               ({ assert(proc, "No proc"); })
#define proc_lock(proc)                 ({ proc_assert(proc); spin_lock(&(proc)->lock); })
#define proc_unlock(proc)               ({ proc_assert(proc); spin_unlock(&(proc)->lock); })
#define proc_trylock(proc)              ({ proc_assert(proc); spin_trylock(&(proc)->lock); })
#define proc_islocked(proc)             ({ proc_assert(proc); spin_islocked(&(proc)->lock); })
#define proc_assert_locked(proc)        ({ proc_assert(proc); spin_assert_locked(&(proc)->lock); })
#define proc_getref(proc)               ({ proc_assert_locked(proc); ++(proc)->refcnt; proc; })
#define proc_putref(proc)               ({ proc_assert_locked(proc); --(proc)->refcnt; })
#define proc_release(proc)              ({ proc_putref(proc); proc_unlock(proc); })

#define proc_setflags(p, f)             ({ proc_assert_locked(p); (p)->flags |=   (f); })
#define proc_unsetflags(p, f)           ({ proc_assert_locked(p); (p)->flags &=   ~(f); })
#define proc_testflags(p, f)            ({ proc_assert_locked(p); (p)->flags &   (f); })
#define proc_isstate(p, st)             ({ proc_assert_locked(p); (p)->state == (st); })
#define proc_isembryo(p)                ({ proc_isstate(p, P_EMBROY); })
#define proc_isrunning(p)               ({ proc_isstate(p, P_RUNNING); })
#define proc_isstopped(p)               ({ proc_isstate(p, P_STOPPED); })
#define proc_iscontinued(p)             ({ proc_isstate(p, P_CONTINUED); })
#define proc_isterminated(p)            ({ proc_isstate(p, P_TERMINATED); })
#define proc_iszombie(p)                ({ proc_isstate(p, P_ZOMBIE); })

#define proc_set_user(p) ({           \
    int locked = 0;                   \
    if ((locked = !proc_islocked(p))) \
        proc_lock(p);                 \
    proc_setflags(p, PROC_USER);      \
    if (locked)                       \
        proc_unlock(p);               \
})

#define proc_unset_user(p) ({         \
    int locked = 0;                   \
    if ((locked = !proc_islocked(p))) \
        proc_lock(p);                 \
    proc_unsetflags(p, PROC_USER);    \
    if (locked)                       \
        proc_unlock(p);               \
})

#define proc_isuser(p) ({                \
    int locked = 0, test = 0;            \
    if ((locked = !proc_islocked(p)))    \
        proc_lock(p);                    \
    test = proc_testflags(p, PROC_USER); \
    if (locked)                          \
        proc_unlock(p);                  \
    test ? 1 : 0;                        \
})

#define proc_sethas_execed(p) ({      \
    int locked = 0;                   \
    if ((locked = !proc_islocked(p))) \
        proc_lock(p);                 \
    proc_setflags(p, PROC_EXECED);    \
    if (locked)                       \
        proc_unlock(p);               \
})

#define proc_unset_has_execed(p) ({   \
    int locked = 0;                   \
    if ((locked = !proc_islocked(p))) \
        proc_lock(p);                 \
    proc_unsetflags(p, PROC_EXECED);  \
    if (locked)                       \
        proc_unlock(p);               \
})

#define proc_hasexeced(p) ({               \
    int locked = 0, test = 0;              \
    if ((locked = !proc_islocked(p)))      \
        proc_lock(p);                      \
    test = proc_testflags(p, PROC_EXECED); \
    if (locked)                            \
        proc_unlock(p);                    \
    test ? 1 : 0;                          \
})

#define proc_set_killed(p) ({         \
    int locked = 0;                   \
    if ((locked = !proc_islocked(p))) \
        proc_lock(p);                 \
    proc_setflags(p, PROC_KILLED);    \
    if (locked)                       \
        proc_unlock(p);               \
})

#define proc_unset_killed(p) ({       \
    int locked = 0;                   \
    if ((locked = !proc_islocked(p))) \
        proc_lock(p);                 \
    proc_unsetflags(p, PROC_KILLED);  \
    if (locked)                       \
        proc_unlock(p);               \
})

#define proc_killed(p) ({                  \
    int locked = 0, test = 0;              \
    if ((locked = !proc_islocked(p)))      \
        proc_lock(p);                      \
    test = proc_testflags(p, PROC_KILLED); \
    if (locked)                            \
        proc_unlock(p);                    \
    test ? 1 : 0;                          \
})

#define proc_mmap(proc)                 ({ proc_assert_locked(proc); (proc)->mmap; })
#define proc_mmap_lock(proc)            ({ mmap_lock(proc_mmap(proc)); })
#define proc_mmap_unlock(proc)          ({ mmap_unlock(proc_mmap(proc)); })
#define proc_mmap_islocked(proc)        ({ mmap_islocked(proc_mmap(proc)); })
#define proc_mmap_assert_locked(proc)   ({ mmap_assert_locked(proc_mmap(proc)); })

extern int procQ_remove(proc_t *proc);
extern int procQ_search_bypid(pid_t pid, proc_t **ref);
extern int procQ_search_bypgid(pid_t pgid, proc_t **ref);

extern void proc_free(proc_t *proc);
extern int proc_init(const char *initpath);
extern int proc_copy(proc_t *child, proc_t *parent);
extern int proc_alloc(const char *name, proc_t **pref);
extern int proc_load(const char *pathname, mmap_t *mmap, thread_entry_t *entry);

/**
 * @brief Describes flags to search for the child.
 * using int proc_get_child();
 */
typedef struct proc_desc_t {
    pid_t           pid;    // Process ID of proc to get.

/// The following are used with desc->pid <= -1 or 0.
/// Only one of these values should be used in desc->flags.

#define SRCH_ANY    0x00    // search for any process.
#define SRCH_STOP   0x01    // search for "any" stopped process.
#define SRCH_CONT   0x02    // search for "any" continued process.
#define SRCH_TERM   0x04    // search for "any" terminated process.
#define SRCH_ZOMB   0x08    // search for "any" Zombie process.
#define SRCH_ALL    0x10    // search for "any" process.
    unsigned        flags;  // Used with pid to provide further info on flags to get proc.
    proc_t          *proc;  // If found, pointer to proc shall be returned through this pointer.

/// Reasons for picking child.

#define CHLD_REAP   BS(0)
#define CHLD_KILL   BS(1)
#define CHLD_STOP   BS(2)
#define CHLD_CONT   BS(3)
#define CHLD_ZOMB   BS(4)
#define CHLD_TERM   BS(5)
#define CHLD_STATE  BS(6)
#define CHLD_EXIST  BS(31)

    unsigned        reason; // if pid == -1, get also why child was picked.
} proc_desc_t;


extern int proc_add_child(proc_t *parent, proc_t *child);
extern int proc_remove_child(proc_t *parent, proc_t *child);
extern int proc_get_child(proc_t *parent, proc_desc_t *desc);
extern int proc_abandon_children(proc_t *new_parent, proc_t *old_parent);