#include <lib/printk.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <arch/cpu.h>
#include <mm/kalloc.h>
#include <lib/string.h>
#include <sys/session.h>
#include <sys/sched.h>
#include <bits/errno.h>

static queue_t *sessions = QUEUE_NEW(/*"All-sessions"*/);

int pgroup_free(PGROUP pgroup) {
    if (!pgroup_islocked(pgroup))
        pgroup_lock(pgroup);

    pgroup_queue_lock(pgroup);
    queue_flush(pgroup_queue(pgroup));
    pgroup_queue_unlock(pgroup);

    assert(0 <= (long)atomic_read(&pgroup->pg_refs), "error pgroup->refs not \'<= 0\'");

    pgroup_unlock(pgroup);
    kfree(pgroup);

    return 0;
}

int pgroup_create(proc_t *leader, PGROUP *pgref) {
    int err = 0;
    pid_t pgid = -ENOMEM;
    pgroup_t *pgroup = NULL;

    proc_assert_locked(leader);
    assert(pgref, "no pgroup ref ptr");

    pgid = leader->pid;

    if ((pgroup = kmalloc(sizeof *pgroup)) == NULL)
        goto error;

    memset(pgroup, 0, sizeof *pgroup);

    pgroup->pg_id       = pgid;
    pgroup->pg_leader   = leader;
    pgroup->pg_queue    = QUEUE_INIT();
    pgroup->pg_lock     = SPINLOCK_INIT();
    atomic_inc(&pgroup->pg_refs);

    pgroup_lock(pgroup);

    if ((err = pgroup_add(pgroup, leader))) {
        pgroup_unlock(pgroup);
        goto error;
    }

    pgroup_unlock(pgroup);

    *pgref = pgroup;
    return 0;
error:
    if (pgroup)
        kfree(pgroup);
    printk("[\e[023453;04mKLOG_WARN\e[0m]: failed to create pgroup\n");
    return err;
}

int pgroup_add(PGROUP pgroup, proc_t *p) {
    int err = -EEXIST;
    proc_assert_locked(p);
    pgroup_assert_locked(pgroup);

    if (pgroup_contains(pgroup, p))
        goto error;

    pgroup_queue_lock(pgroup);
    if ((err = enqueue(pgroup_queue(pgroup), p, 1, NULL))) {
        pgroup_queue_unlock(pgroup);
        goto error;
    }
    pgroup_queue_unlock(pgroup);

    p->pgroup = pgroup;

    return 0;
error:
    printk("[\e[023453;04mKLOG_WARN\e[0m]: failed to add proc to pgroup, error=%d\n", err);
    return err;
}

int pgroup_lookup(PGROUP pgroup, pid_t pid, proc_t **ref) {
    proc_t *p = NULL;
    assert(ref, "no proc ref");
    pgroup_assert_locked(pgroup);

    pgroup_queue_lock(pgroup);
    forlinked(node, pgroup_queue(pgroup)->head, node->next) {
        p = node->data;
        proc_lock(p);
        if (p->pid == pid) {
            *ref = p;
            proc_unlock(p);
            pgroup_queue_unlock(pgroup);
            return 0;
        }
        proc_unlock(p);
    }
    pgroup_queue_unlock(pgroup);
    return -ENOENT;
}

int pgroup_contains(PGROUP pgroup, proc_t *p) {
    proc_assert(p);
    pgroup_assert_locked(pgroup);

    pgroup_queue_lock(pgroup);
    forlinked(node, pgroup_queue(pgroup)->head, node->next) {
        if (p == (proc_t *)node->data) {
            pgroup_queue_unlock(pgroup);
            return 1;
        }
    }
    pgroup_queue_unlock(pgroup);
    return 0;
}

int pgroup_remove(PGROUP pgroup, proc_t *p) {
    int err = 0;
    proc_assert_locked(p);
    pgroup_assert_locked(pgroup);

    if (pgroup_contains(pgroup, p) == 0) {
        err = -ENOENT;
        goto error;
    }

    pgroup_queue_lock(pgroup);
    if ((err = queue_remove(pgroup_queue(pgroup), p))) {
        pgroup_queue_unlock(pgroup);
        goto error;
    }
    pgroup_queue_unlock(pgroup);

    p->pgroup = NULL;

    return 0;
error:
    printk("[\e[023453;04mKLOG_WARN\e[0m]: failed to remove proc from pgroup, error=%d\n", err);
    return err;
}

int pgroup_leave(PGROUP pgroup) {
    pgroup_lock(pgroup);
    int err = pgroup_remove(pgroup, curproc);
    pgroup_unlock(pgroup);
    return err;
}

int session_free(SESSION session) {
    if (!session_islocked(session))
        session_lock(session);

    queue_lock(sessions);
    queue_remove(sessions, session);
    queue_unlock(sessions);

    session_queue_lock(session);
    queue_flush(session_queue(session));
    session_queue_unlock(session);

    assert(0 <= (long)atomic_read(&session->ss_refs), "error session->refs not \'<= 0\'");

    session_unlock(session);

    kfree(session);
    return 0;
}

int session_create(proc_t *leader, SESSION *ref) {
    int err = 0;
    pid_t sid = 0;
    PGROUP pgroup = NULL;
    SESSION session = NULL;

    proc_assert_locked(leader);
    assert(ref, "no session ref");

    sid = leader->pid;

    if ((session = kmalloc(sizeof *session)) == NULL) {
        err = -ENOMEM;
        goto error;
    }

    memset(session, 0, sizeof *session);

    session->ss_sid     = sid;
    session->ss_leader  = leader;
    session->ss_queue   = QUEUE_INIT();
    session->ss_lock    = SPINLOCK_INIT();
    atomic_inc(&session->ss_refs);

    session_lock(session);
    if ((err = session_create_pgroup(session, leader, &pgroup))) {
        session_unlock(session);
        goto error;
    }
    session_unlock(session);

    *ref = session;

    queue_lock(sessions);
    if ((err = enqueue(sessions, session, 1, NULL))) {
        queue_unlock(sessions);
        goto error;
    }
    queue_unlock(sessions);

    return 0;
error:
    if (session)
        kfree(session);
    if (pgroup)
        pgroup_free(pgroup);
    printk("[\e[023453;04mKLOG_WARN\e[0m]: failed to create session, error=%d\n", err);
    return err;
}

int session_create_pgroup(SESSION session, proc_t *leader, PGROUP *ref) {
    int err = 0;
    PGROUP pgroup = NULL;

    session_assert_locked(session);
    proc_assert_locked(leader);
    assert(ref, "no pgroup ref");

    if ((err = pgroup_create(leader, &pgroup)))
        goto error;
    
    pgroup_lock(pgroup);
    //printk("created PGID: %d\n", pgroup->pg_id);
    if ((err = session_add(session, pgroup))) {
        pgroup_unlock(pgroup);
        goto error;
    }
    pgroup_unlock(pgroup);

    leader->session = session;
    *ref = pgroup;
    return 0;
error:
    if (pgroup)
        pgroup_free(pgroup);
    printk("[\e[023453;04mKLOG_WARN\e[0m]: failed to create pgroup in session, error=%d\n", err);
    return err;
}

int session_add(SESSION session, PGROUP pgroup) {
    int err = 0;
    session_assert_locked(session);
    pgroup_assert_locked(pgroup);

    if (session_contains(session, pgroup)) {
        err = -EEXIST;
        goto error;
    }

    session_queue_lock(session);
    if ((err = enqueue(session_queue(session), pgroup, 1, NULL))) {
        err = -ENOMEM;
        session_queue_unlock(session);
        goto error;
    }
    session_queue_unlock(session);

    pgroup->pg_sessoin = session;
    
    pgroup_incr(pgroup);

    return 0;
error:
    printk("[\e[023453;04mKLOG_WARN\e[0m]: failed add pgroup to session, error=%d\n", err);
    return err;
}

int session_contains(SESSION session, PGROUP pgroup) {
    session_assert_locked(session);
    pgroup_assert(pgroup);

    session_queue_lock(session);
    forlinked(node, session_queue(session)->head, node->next) {
        if (pgroup == (PGROUP)node->data) {
            session_queue_unlock(session);
            return 1;
        }
    }
    session_queue_unlock(session);

    return 0;
}

int session_lookup(SESSION session, pid_t pgid, PGROUP *ref) {
    PGROUP pgroup = NULL;
    session_assert_locked(session);
    assert(ref, "no pgroup ref");

    session_queue_lock(session);
    forlinked(node, session_queue(session)->head, node->next) {
        pgroup = node->data;
        pgroup_lock(pgroup);
        if (pgroup->pg_id == pgid) {
            *ref = pgroup;
            session_queue_unlock(session);
            return 0;
        }
        pgroup_unlock(pgroup);
    }
    session_queue_unlock(session);

    return -ENOENT;
}

int session_remove(SESSION session, PGROUP pgroup) {
    int err = -ENOENT;
    session_assert_locked(session);
    pgroup_assert_locked(pgroup);

    if (session_contains(session, pgroup) == 0)
        goto error;

    session_queue_lock(session);
    if ((err = queue_remove(session_queue(session), pgroup))) {
        session_queue_unlock(session);
        goto error;
    }
    session_queue_unlock(session);

    pgroup->pg_sessoin = NULL;
    return 0;
error:
    printk("[\e[023453;04mKLOG_WARN\e[0m]: failed to remove pgroup from session, error=%d\n", err);
    return err;
}

int session_leave(SESSION session, proc_t *p) {
    int err = 0, count = 0;
    PGROUP pgroup = NULL;

    proc_assert_locked(p);
    session_assert_locked(session);
    pgroup = p->pgroup;

    if (session_contains(session, pgroup) == 0) {
        err = -ENOENT;
        goto error;
    }

    pgroup_lock(pgroup);

    if ((err = pgroup_remove(pgroup, p))) {
        pgroup_unlock(pgroup);
        goto error;
    }

    p->session = NULL;

    pgroup_queue_lock(pgroup);
    if ((count = queue_count(pgroup_queue(pgroup))) == 0) {
        if ((err = session_remove(session, pgroup))) {
            pgroup_unlock(pgroup);
            goto error;
        }

        //printk("[\e[023453;04mKLOG_WARN\e[0m]: removed pgroup(%d) from session(%d)\n", pgroup->pg_id, session->ss_sid);
        pgroup_queue_unlock(pgroup);
        pgroup_unlock(pgroup);
        pgroup_free(pgroup);
        return 0;
    }

    //printk("%d processes remaining in pgroup(%d)\n", count, pgroup->pg_id);
    pgroup_queue_unlock(pgroup);
    pgroup_unlock(pgroup);
    return 0;
error:
    printk("[\e[023453;04mKLOG_WARN\e[0m]: failed to leave session, error=%d\n", err);
    return err;
}

int session_pgroup_join(SESSION session, pid_t pgid, proc_t *process) {
    int err = 0;
    PGROUP pgroup = NULL;

    if (pgid < 0) return -EINVAL;

    proc_assert_locked(curproc);
    proc_assert_locked(process);
    session_assert_locked(session);

    if ((err = session_lookup(session, pgid, &pgroup)))
        goto error;
    
    if (process->pgroup == pgroup) // Already in this pgroup
        goto done;

    // found the pgroup with PGID == pgid
    
    /*process shouldn't belong to any session or a pgroup*/
    if ((err = session_exit(session, process, 0))) {
        pgroup_unlock(pgroup);
        goto error;
    }

    if ((err = pgroup_add(pgroup, process))) {
        pgroup_unlock(pgroup);
        goto error;
    }
    
done:
    //klog(KLOG_OK, "process(%d) successfully joined pgroup(%d)\n", process->pid, pgroup->pg_id);
    pgroup_unlock(pgroup);
    process->session = session;
    return 0;
error:
    printk("[\e[023453;04mKLOG_WARN\e[0m]: failed to join PGID(%d), error: %d\n", pgid, err);
    return err;
}

ssize_t session_pgroup_count(SESSION session) {
    ssize_t count = 0;
    session_assert_locked(session);
    session_queue_lock(session);
    count = queue_count(session_queue(session));
    session_queue_unlock(session);
    return count;
}

int session_exit(SESSION session, proc_t *process, int free_session) {
    int err = 0, locked = 0;
    ssize_t count = 0;

    proc_assert_locked(process);

    if ((locked = !session_islocked(session)))
        session_lock(session);

    // make sure process is in this session
    if (session != process->session) {
        err = -EINVAL;
        if (locked) session_unlock(session);
        goto error;
    }

    // leave this session
    if ((err = session_leave(session, process))) {
        if (locked) session_unlock(session);
        goto error;
    }
    
    // If there are no more pgroups in this session free it
    count = session_pgroup_count(session);
    if ((count <= 0) && free_session) {
        printk("[\e[023453;04mKLOG_WARN\e[0m]: freeing session(count: %d)\n", count);
        session_free(session);
        return 0;
    }

    //printk("[\e[023453;04mKLOG_WARN\e[0m]: session(count: %d), free: %d\n", count, free_session);

    if (locked) session_unlock(session);
    return 0;
error:
    printk("[\e[023453;04mKLOG_WARN\e[0m]: failed to exit session, error: %d\n", err);
    return err;
}

int sessions_get(pid_t sid, SESSION *ref) {
    SESSION session = NULL;
    queue_lock(sessions);
    forlinked(node, sessions->head, node->next) {
        session = node->data;
        session_lock(session);
        if (session->ss_sid == sid) {
            if (ref) {
                *ref = session;
                queue_unlock(sessions);
                return 0;
            }
            session_unlock(session);
            queue_unlock(sessions);
            return 0;
        }
        session_unlock(session);
    }
    queue_unlock(sessions);
    return -ESRCH;
}

int sessions_find_pgroup(pid_t pgid, PGROUP *ref) {
    PGROUP pgroup = NULL;
    SESSION session = NULL;

    queue_lock(sessions);
    forlinked(node, sessions->head, node->next) {
        session = node->data;
        session_lock(session);
        
        if (session_lookup(session, pgid, &pgroup) == 0) {
            if (ref) {
                *ref = pgroup;
                session_unlock(session);
                queue_unlock(sessions);
                return 0;
            }

            pgroup_unlock(pgroup);
            session_unlock(session);
            queue_unlock(sessions);
            return 0;
        }

        session_unlock(session);
    }
    queue_unlock(sessions);
    return -ESRCH;
}

int session_end(SESSION session);
int session_pgroup_remove(SESSION session);