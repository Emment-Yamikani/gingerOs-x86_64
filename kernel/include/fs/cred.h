#pragma once

#include <lib/types.h>
#include <sync/spinlock.h>

typedef struct cred_t {
    uid_t   c_uid;
    uid_t   c_euid;
    uid_t   c_suid;
    gid_t   c_gid;
    gid_t   c_egid;
    gid_t   c_sgid;
    mode_t  c_umask;
    spinlock_t c_lock;
} cred_t;

#define CRED_DEFAULT() ((cred_t){\
    .c_uid  = 0,                \
    .c_euid = 0,                \
    .c_egid = 0,                \
    .c_gid  = 0,                \
    .c_suid = 0,                \
    .c_sgid = 0,                \
    .c_lock = SPINLOCK_INIT()   \
})


#define cred_assert(cr)         ({ assert(cr, "No credentials!\n"); })
#define cred_islocked(cr)       ({ cred_assert(cr); spin_islocked(&(cr)->c_lock); })
#define cred_assert_locked(cr)  ({ cred_assert(cr); spin_assert_locked(&(cr)->c_lock); })
#define cred_lock(cr)           ({ cred_assert(cr); spin_lock(&(cr)->c_lock); })
#define cred_unlock(cr)         ({ cred_assert(cr); spin_unlock(&(cr)->c_lock); })
#define cred_trylock(cr)        ({ cred_assert(cr); spin_trylock(&(cr)->c_lock); })

int cred_copy(cred_t *dst, cred_t *src);