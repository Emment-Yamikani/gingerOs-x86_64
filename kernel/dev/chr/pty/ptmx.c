#include <bits/errno.h>
#include <dev/pty.h>
#include <dev/dev.h>
#include <sync/atomic.h>
#include <sys/system.h>
#include <lib/string.h>

// pseudoterminal devices.
static pty_t       pseudo_terms[NPTMX] = {0};
static SPINLOCK(pseudo_termslk);

#define pseudo_lock()           spin_lock(pseudo_termslk)
#define pseudo_unlock()         spin_unlock(pseudo_termslk)
#define pseudo_islocked()       spin_islocked(pseudo_termslk)
#define pseudo_assert_locked()  spin_assert_locked(pseudo_termslk)


int pseudo_init(void) {
    memset(pseudo_terms, 0, sizeof pseudo_terms);
    *pseudo_termslk = SPINLOCK_INIT();
    return 0;
}

int ptmx_alloc(PTY *ref) {
    int         err     = 0;
    PTY         pty     = NULL;
    ringbuf_t   *slave  = NULL;
    ringbuf_t   *master = NULL;

    if (ref == NULL)
        return -EINVAL;
    
    pseudo_lock();

    for (pty = pseudo_terms; pty < &pseudo_terms[NPTMX]; ++pty) {
        if ((pty->pt_flags & PTY_USED) == 0) {
            if ((err = ringbuf_new(PTY_BUFFER_SIZE, &master)))
                return err;
            
            if ((err = ringbuf_new(PTY_BUFFER_SIZE, &slave))) {
                ringbuf_free(master);
                return err;
            }

            pty->pt_refs = 1;
            pty->slave   = slave;
            pty->master  = master;
            pty->pt_lock = SPINLOCK_INIT(); 
            pty->pt_id   = pty - pseudo_terms;
            pty->pt_flags |= PTY_USED | PTY_LOCKED;
            
            pty_lock(pty);

            *ref = pty;
            pseudo_unlock();
            return 0;
        }
    }

    pseudo_unlock();

    return -ENOMEM;
}

void ptmx_free(PTY pty) {
    pty_assert(pty);

    pseudo_lock();

    if (!pty_islocked(pty))
        pty_lock(pty);

    if (--pty->pt_refs <= 0) {
        pty->pt_refs  = 0;
        pty->pt_flags = 0; // mark it free.
        ringbuf_free(pty->slave);
        ringbuf_free(pty->master);
    }

    pty_unlock(pty);
    pseudo_unlock();
}

int pty_find(int id, PTY *pp) {
    PTY pty = NULL;

    if (pp == NULL)
        return -EINVAL;
    
    pseudo_lock();
    for (pty = pseudo_terms; pty < &pseudo_terms[NPTMX]; ++pty) {
        if (pty->pt_id == id) {
            pty_lock(pty);
            pty->pt_refs++;
            *pp = pty;
            return 0;
        }
    }
    pseudo_unlock();

    return -ENOENT;
}