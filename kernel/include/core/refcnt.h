#include <sync/atomic.h>
#include <sync/spinlock.h>

typedef struct refcnt_t {
    atomic_t    ref_cnt;    // Reference count
    spinlock_t  ref_lock;   // Indicates if the object is valid
} refcnt_t;

#define refcnt_assert(rc)   ({ assert(rc, "Invalid ptr to refcnt."); })
#define refcnt_lock(rc)     ({ refcnt_assert(rc); spin_lock(&(rc)->ref_lock); })
#define refcnt_unlock(rc)   ({ refcnt_assert(rc); spin_unlock(&(rc)->ref_lock); })

static inline void refcnt_init(refcnt_t *ref) {
    refcnt_assert(ref);
    ref->ref_cnt  = 1;
    ref->ref_lock = SPINLOCK_INIT();
}

#define refcnt_dup(ptr) do {                        \
        refcnt_lock(ptr);                           \
        if ((ptr)->ref_cnt <= 0) {                  \
            refcnt_unlock(ptr);                     \
            panic("Invalid reference to object.");  \
        } else (ptr)->ref_cnt++;                    \
        refcnt_unlock((ptr));                       \
    } while (0)

#define refcnt_put(ptr, type, member, clean_up) do {            \
    refcnt_lock(ptr);                                           \
    (ptr)->ref_cnt--;                                           \
    if ((ptr)->ref_cnt <= 0) {                                  \
        refcnt_unlock(ptr);                                     \
        type *container = container_of((ptr), type, member);    \
        if ((clean_up) != 0) (clean_up)(container);             \
    } else refcnt_unlock(ptr);                                  \
} while (0)
