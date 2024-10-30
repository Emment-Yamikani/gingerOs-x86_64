#pragma once

#include <dev/pty.h>
#include <ds/queue.h>
#include <ds/ringbuf.h>
#include <fs/inode.h>

#define NPTMX    256 // total number of allowed pseudoterminal masters.

// pseudoterminal
typedef struct pty_t {
    int     pt_id;          // psuedoterminal ID.
    inode_t *pt_imaster;    // master inode.
    inode_t *pt_islave;     // slave inode.

    #define PTY_LOCKED  0x1 // is pseudoterminal locked?
    u32     pt_flags;       // pseudoterminal flags.

    /** @brief TODO: define a struct termio
     *  and define an instance here.*/
    spinlock_t pt_lock;     // lock
} pty_t;

#define pty_assert(pt)          ({ assert(pt, "No PTY."); })

#define pty_lock(pt)            ({ pty_assert(pt); spin_lock(&(pt)->pt_lock); })
#define pty_unlock(pt)          ({ pty_assert(pt); spin_unlock(&(pt)->pt_lock); })
#define pty_islocked(pt)        ({ pty_assert(pt); spin_islocked(&(pt)->pt_lock); })
#define pty_assert_locked(pt)   ({ pty_assert(pt); spin_assert_locked(&(pt)->pt_lock); })

