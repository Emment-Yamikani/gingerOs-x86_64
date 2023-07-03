#pragma once

#include <ds/list.h>
#include <lib/stdint.h>
#include <sync/assert.h>
#include <sync/spinlock.h>

struct inode;

typedef struct dentry
{
    list_head_t     d_list;
    list_head_t     d_alias;

    char            *d_name;
    unsigned long   d_flags;
    long            d_count;

    struct dentry   *d_prev;
    struct dentry   *d_next;
    struct dentry   *d_child;
    struct dentry   *d_parent;
    
    struct inode    *d_inode;

    spinlock_t      d_lock;
} dentry_t;

#define dentry_assert(dentry)           ({ assert(dentry, "No dentry"); })
#define dentry_lock(dentry)             ({ dentry_assert(dentry); spin_lock(&dentry->d_lock); })
#define dentry_unlock(dentry)           ({ dentry_assert(dentry); spin_unlock(&dentry->d_lock); })
#define dentry_assert_locked(dentry)    ({ dentry_assert(dentry); spin_assert_locked(&dentry->d_lock); })

int dentry_cache(void);
int dentry_dup(dentry_t *dentry);
list_head_t *dcache_list_get(void);
void dentry_close(dentry_t *dentry);
void dentry_release(dentry_t *dentry);
dentry_t *dentry_alloc(const char *name);
int dentry_bind(dentry_t *d_parent, dentry_t *d_child);
int dentry_find(dentry_t *d_parent, const char *name, dentry_t **d_child);