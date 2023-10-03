#pragma once

#include <lib/stdint.h>
#include <lib/stdlib.h>
#include <lib/printk.h>
#include <fs/inode.h>
#include <sync/spinlock.h>
#include <ds/stack.h>

#define DCACHE_MOUNTED      1
#define DCACHE_CAN_FREE     2
#define DCACHE_REFERENCED   4

struct dentry;

typedef struct{
    void (*diput)(struct dentry *dp);
    int  (*ddelete)(struct dentry *dp);
    void (*drelease)(struct dentry *dp);
    int  (*drevalidate)(struct dentry *dp);
} dops_t;

typedef struct dentry {
    char            *d_name;    // dentry's name.
    unsigned long   d_flags;    // dentry's flags.
    inode_t         *d_inode;   // dentry's inode.
    long            d_count;    // dentry's ref count.
    dops_t          d_ops;      // dentry's operations.
    stack_t         *d_mnt_stack;// dentry's mountpoint stack.
    struct dentry   *d_next;    // dentry's next sibling.
    struct dentry   *d_prev;    // dentry's prev sibling.
    struct dentry   *d_parent;  // dentry's parent.
    struct dentry   *d_child;   // dentry's first child, if dentry is a directory.
    spinlock_t      d_lock;     // dentry's spinlock.
} dentry_t;

#define dassert(dentry) ({                \
    assert((dentry), "No dentry struct"); \
})

#define dassert_locked(dentry) ({          \
    dassert(dentry);                       \
    spin_assert_locked(&(dentry)->d_lock); \
})

#define dislocked(dentry) ({            \
    spin_islocked(&(dentry)->d_lock); \
})

#define dlock(dentry) ({          \
    dassert(dentry);              \
    spin_lock(&(dentry)->d_lock); \
})

#define dunlock(dentry) ({          \
    dassert(dentry);                \
    spin_unlock(&(dentry)->d_lock); \
})

#define dtestflags(dentry, flags) ({ \
    (dentry)->d_flags & (flags);   \
})

#define dsetflags(dentry, flags) ({ \
    (dentry)->d_flags |= (flags);   \
})

#define dunsetflags(dentry, flags) ({ \
    (dentry)->d_flags &= ~(flags);    \
})

void ddump(dentry_t *dentry, int flags);
#define DDUMP_HANG  1
#define DDUMP_PANIC 2

int dmkdentry(dentry_t *dir, const char *name, dentry_t **pdp);
void dput(dentry_t *dp);
void ddup(dentry_t *dentry);
void dclose(dentry_t *dentry);
void dunbind(dentry_t *dentry);
long dget_count(dentry_t *dp);
void drelease(dentry_t *dentry);
int dbind(dentry_t *parent, dentry_t *child);
int dalloc(const char *name, dentry_t **pdentry);
int dlookup(dentry_t *dir, const char *name, dentry_t **pchild);