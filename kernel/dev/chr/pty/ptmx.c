#include <bits/errno.h>
#include <dev/pty.h>
#include <dev/dev.h>
#include <sync/atomic.h>
#include <sys/system.h>

__unused static atomic_t    pt_ids = 0; 
__unused static pty_t       pseudo_terms[NPTMX];
__unused static SPINLOCK(pseudo_termslk);

__unused static atomic_t ptmx_alloc_id(void) {
    return atomic_fetch_inc(&pt_ids);
}

