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
    EMBROY,
    RUNNING,
    ZOMBIE,
    TERMINATED,
} status_t;

typedef struct proc {
    pid_t            pid;       // process' ID.
    pid_t            pgroup;    // process' group
    pid_t            session;   // process' session
    struct proc     *parent;    // process' parent.
    
    status_t         state;    // process' status.
    long             flags;     // process' flags.
    long             exit_code; // process' exit status.
    thread_entry_t   entry;     // process' entry point.
    long             refcnt;    // process' reference count.
    tgroup_t        *tgroup;    // process' thread group.
    
    char            *name;      // process' name.
    mmap_t          *mmap;      // process' memory map(virtual address space).
    
    cond_t           wait;      // process' wait condition.
    queue_t         children;   // process' children queue.

    spinlock_t      lock;       // lock to protect this structure.
} proc_t;

#define PROC_USER               BS(0)   // process is a user process.
#define PROC_EXECED             BS(1)   // process has executed exec().
#define PROC_KILLED             BS(2)   // process killed.
#define PROC_ORPHANED           BS(3)   // process was orphaned by parent.

#define NPROC                   (32786)
#define curproc                 ({ current ? current->t_owner : NULL; })                //

extern queue_t *procQ;

/// INIT process of the system.
extern proc_t *initproc;

#define proc_assert(proc)               ({ assert(proc, "No proc"); })
#define proc_lock(proc)                 ({ proc_assert(proc); spin_lock(&(proc)->lock); })
#define proc_unlock(proc)               ({ proc_assert(proc); spin_unlock(&(proc)->lock); })
#define proc_trylock(proc)              ({ proc_assert(proc); spin_trylock(&(proc)->lock); })
#define proc_islocked(proc)             ({ proc_assert(proc); spin_islocked(&(proc)->lock); })
#define proc_assert_locked(proc)        ({ proc_assert(proc); spin_assert_locked(&(proc)->lock); })
#define proc_getref(proc)               ({ proc_assert_locked(proc); (proc)->refcnt++; proc; })
#define proc_putref(proc)               ({ proc_assert_locked(proc); (proc)->refcnt--; })
#define proc_release(proc)              ({ proc_assert_locked(proc); (proc)->refcnt--; proc_unlock(proc); })

#define proc_setflags(p, f)             ({ proc_assert_locked(p); (p)->flags |= (f); })
#define proc_unsetflags(p, f)           ({ proc_assert_locked(p); (p)->flags &= ~(f); })
#define proc_testflags(p, f)            ({ proc_assert_locked(p); (p)->flags & (f); })

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

#define proc_unset_has_execed(p) ({    \
    int locked = 0;                   \
    if ((locked = !proc_islocked(p))) \
        proc_lock(p);                 \
    proc_unsetflags(p, PROC_EXECED);  \
    if (locked)                       \
        proc_unlock(p);               \
})

#define proc_hasexeced(p) ({              \
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

#define proc_tgroup(proc)               ({ proc_assert_locked(proc); (proc)->tgroup; })


#define proc_tgroup_lock(proc)          ({ tgroup_lock(proc_tgroup(proc)); })
#define proc_tgroup_unlock(proc)        ({ tgroup_unlock(proc_tgroup(proc)); })
#define proc_tgroup_islocked(proc)      ({ tgroup_islocked(proc_tgroup(proc)); })
#define proc_tgroup_assert_locked(proc) ({ tgroup_assert_locked(proc_tgroup(proc)); })

#define proc_mmap(proc)                 ({ proc_assert_locked(proc); (proc)->mmap; })
#define proc_mmap_lock(proc)            ({ mmap_lock(proc_mmap(proc)); })
#define proc_mmap_unlock(proc)          ({ mmap_unlock(proc_mmap(proc)); })
#define proc_mmap_islocked(proc)        ({ mmap_islocked(proc_mmap(proc)); })
#define proc_mmap_assert_locked(proc)   ({ mmap_assert_locked(proc_mmap(proc)); })

extern int procQ_remove(proc_t *proc);
extern int procQ_search_bypid(pid_t pid, proc_t **ref);
extern int procQ_search_bypgid(pid_t pgid, proc_t **ref);

extern int proc_init(const char *initpath);
extern void proc_free(proc_t *proc);
extern int  proc_alloc(const char *name, proc_t **pref);
extern int proc_copy(proc_t *child, proc_t *parent);

/**
 * @brief Describes how to search for the child.
 * using int proc_get_child();
 */
typedef struct child_desc_t {
    pid_t   pid;    // Process ID of child to get.
    int     flags;  // Used with pid to provide further info on how to get child.
    proc_t  *child; // If found, pointer to a child shall be returned through this pointer.
} child_desc_t;

#define CHLD_ANY            // Any child of calling process.
#define CHLD_SIG            // Child that was signaled and didn't catch the signal.
#define CHLD_ZOMBIE         // Child is in a zombie state.

extern int proc_add_child(proc_t *parent, proc_t *child);
extern int proc_remove_child(proc_t *parent, proc_t *child);
extern int proc_get_child(proc_t *parent, child_desc_t *desc);
extern int proc_abandon_children(proc_t *new_parent, proc_t *old_parent);