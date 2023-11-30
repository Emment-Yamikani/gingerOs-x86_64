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

typedef struct proc {
    char            *name;      // process' name.
    pid_t            pid;       // process' ID.
    uintptr_t        entry;     // process' entry point.
    long             exit;      // process' exit status.
    mmap_t          *mmap;      // process' memory map(virtual address space).
    cond_t           wait;      // process' wait condition.
    tgroup_t        *tgroup;    // process' thread group.
    queue_t         *children;  // process' children queue.
    struct pgroup   *pgroup;    // process' group
    struct session  *session;   // process' session

    spinlock_t      lock;       // lock to protect this structure.
} proc_t;

#define curproc                 ({ current ? current->t_owner : NULL; })                //

#define proc_assert(p)          ({ assert(p, "No proc"); })
#define proc_lock(p)            ({ proc_assert(p); spin_lock(&(p)->lock); })
#define proc_unlock(p)          ({ proc_assert(p); spin_unlock(&(p)->lock); })
#define proc_trylock(p)         ({ proc_assert(p); spin_trylock(&(p)->lock); })
#define proc_islocked(p)        ({ proc_assert(p); spin_islocked(&(p)->lock); })
#define proc_assert_locked(p)   ({ proc_assert(p); spin_assert_locked(&(p)->lock); })

extern int  proc_init(proc_t *proc);
extern void proc_free(proc_t *proc);
extern int  proc_alloc(const char *name, proc_t **pref);