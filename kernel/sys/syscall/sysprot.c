#include <fs/fs.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <sys/sysprot.h>
#include <sys/thread.h>

uid_t getuid(void) {
    uid_t           uid     = 0;
    file_table_t    *ft     = NULL;
    cred_t          *cred   = NULL;

    assert(current_tgroup(), "No current_tgroup.");

    current_lock();
    current_tgroup_lock();
    ft = &current_tgroup()->tg_file_table;
    cred = &ft->cred;
    uid = cred->c_uid;
    current_tgroup_unlock();
    current_unlock();
    return uid;
}

gid_t getgid(void) {
    gid_t            gid    = 0;
    file_table_t    *ft     = NULL;
    cred_t          *cred   = NULL;

    assert(current_tgroup(), "No current_tgroup.");

    current_lock();
    current_tgroup_lock();
    ft = &current_tgroup()->tg_file_table;
    cred = &ft->cred;

    gid = cred->c_gid;
    current_tgroup_unlock();
    current_unlock();
    return gid;
}

uid_t geteuid(void) {
    uid_t uid = 0;
    cred_t *cred = NULL;
    file_table_t *ft = NULL;

    assert(current_tgroup(), "No current_tgroup.");

    current_lock();
    current_tgroup_lock();
    ft = &current_tgroup()->tg_file_table;
    cred = &ft->cred;

    uid = cred->c_euid;
    current_tgroup_unlock();
    current_unlock();
    return uid;
}

gid_t getegid(void) {
    gid_t            gid    = 0;
    file_table_t    *ft     = NULL;
    cred_t          *cred   = NULL;

    assert(current_tgroup(), "No current_tgroup.");

    current_lock();
    current_tgroup_lock();
    ft = &current_tgroup()->tg_file_table;
    cred = &ft->cred;

    gid = cred->c_egid;
    current_tgroup_unlock();
    current_unlock();
    return gid;
}

int setuid(uid_t uid) {
    int             err     = 0;
    file_table_t    *ft     = NULL;
    cred_t          *cred   = NULL;

    assert(current_tgroup(), "No current_tgroup().");

    current_lock();
    current_tgroup_lock();
    ft = &current_tgroup()->tg_file_table;
    cred = &ft->cred;

    /*No. permissions.*/
    if (uid != cred->c_uid && cred->c_uid != 0)
        err = -EPERM;
    else
        cred->c_uid = uid;

    current_tgroup_unlock();
    current_unlock();
    return err;
}

int setgid(gid_t gid) {
    int          err    = 0;
    file_table_t *ft    = NULL;
    cred_t       *cred  = NULL;

    assert(current_tgroup(), "No current_tgroup().");

    current_lock();
    current_tgroup_lock();
    ft = &current_tgroup()->tg_file_table;
    cred = &ft->cred;

    /*No. permissions.*/
    if (gid != cred->c_gid && cred->c_gid != 0)
        err = -EPERM;
    else
        cred->c_gid = gid;

    current_tgroup_unlock();
    current_unlock();
    return err;
}

int seteuid(uid_t euid) {
    int             err     = 0;
    file_table_t    *ft     = NULL;
    cred_t          *cred   = NULL;

    assert(current_tgroup(), "No current_tgroup().");

    current_lock();
    current_tgroup_lock();
    ft = &current_tgroup()->tg_file_table;
    cred = &ft->cred;

    /*No. permissions.*/
    if (euid != cred->c_euid && cred->c_euid != 0)
        err = -EPERM;
    else
        cred->c_euid = euid;

    current_tgroup_unlock();
    current_unlock();
    return err;
}

int setegid(gid_t egid) {
    int             err     = 0;
    file_table_t    *ft     = NULL;
    cred_t          *cred   = NULL;

    assert(current_tgroup(), "No current_tgroup().");

    current_lock();
    current_tgroup_lock();
    ft = &current_tgroup()->tg_file_table;
    cred = &ft->cred;

    /*No. permissions.*/
    if (egid != cred->c_egid && cred->c_egid != 0)
        err = -EPERM;
    else
        cred->c_egid = egid;

    current_tgroup_unlock();
    current_unlock();
    return err;
}