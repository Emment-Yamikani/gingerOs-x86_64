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
    thread_entry_t   entry;     // process' entry point.
    long             refcnt;    // process' reference count.
    long             exit;      // process' exit status.
    mmap_t          *mmap;      // process' memory map(virtual address space).
    cond_t           wait;      // process' wait condition.
    tgroup_t        *tgroup;    // process' thread group.
    queue_t         children;   // process' children queue.
    struct pgroup   *pgroup;    // process' group
    struct session  *session;   // process' session

    spinlock_t      lock;       // lock to protect this structure.
} proc_t;

#define NPROC                   (32786)
#define curproc                 ({ current ? current->t_owner : NULL; })                //

/// INIT process of the system.
extern proc_t *initproc;

#define proc_assert(proc)               ({ assert(proc, "No proc"); })
#define proc_lock(proc)                 ({ proc_assert(proc); spin_lock(&(proc)->lock); })
#define proc_unlock(proc)               ({ proc_assert(proc); spin_unlock(&(proc)->lock); })
#define proc_trylock(proc)              ({ proc_assert(proc); spin_trylock(&(proc)->lock); })
#define proc_islocked(proc)             ({ proc_assert(proc); spin_islocked(&(proc)->lock); })
#define proc_assert_locked(proc)        ({ proc_assert(proc); spin_assert_locked(&(proc)->lock); })
#define proc_getref(proc)               ({ proc_assert_locked(proc); (proc)->refcnt++; })
#define proc_release(proc)              ({ proc_assert_locked(proc); (proc)->refcnt--; proc_unlock(proc); })

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

extern int proc_init(const char *initpath);
extern void proc_free(proc_t *proc);
extern int  proc_alloc(const char *name, proc_t **pref);