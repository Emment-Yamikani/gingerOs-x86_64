#include <fs/fs.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <sys/sysprot.h>
#include <sys/thread.h>
#include <mm/kalloc.h>

uid_t getuid(void) {
    uid_t           uid     = 0;
    cred_t          *cred   = NULL;
    file_table_t    *file_table = NULL;

    current_lock();
    file_table = current->t_file_table;

    ftlock(file_table);
    cred = &file_table->cred;
    uid = cred->c_uid;
    ftunlock(file_table);

    current_unlock();
    return uid;
}

gid_t getgid(void) {
    gid_t            gid    = 0;
    cred_t          *cred   = NULL;
    file_table_t    *file_table = NULL;

    current_lock();
    file_table = current->t_file_table;

    ftlock(file_table);
    cred = &file_table->cred;
    gid = cred->c_gid;
    ftunlock(file_table);

    current_unlock();
    return gid;
}

uid_t geteuid(void) {
    uid_t uid = 0;
    cred_t *cred = NULL;
    file_table_t *file_table = NULL;

    current_lock();
    file_table = current->t_file_table;
    
    ftlock(file_table);
    cred = &file_table->cred;
    uid = cred->c_euid;
    ftunlock(file_table);

    current_unlock();
    return uid;
}

gid_t getegid(void) {
    gid_t            gid    = 0;
    cred_t          *cred   = NULL;
    file_table_t    *file_table = NULL;

    current_lock();
    file_table = current->t_file_table;

    ftlock(file_table);
    cred = &file_table->cred;
    gid = cred->c_egid;
    ftunlock(file_table);

    current_unlock();
    return gid;
}

int setuid(uid_t uid) {
    int             err     = 0;
    cred_t          *cred   = NULL;
    file_table_t    *file_table = NULL;

    current_lock();
    file_table = current->t_file_table;
    
    ftlock(file_table);
    cred = &file_table->cred;

    /*No. permissions.*/
    if (uid != cred->c_uid && cred->c_uid != 0)
        err = -EPERM;

    else
        cred->c_uid = uid;

    ftunlock(file_table);

    current_unlock();
    return err;
}

int setgid(gid_t gid) {
    int          err    = 0;
    cred_t       *cred  = NULL;
    file_table_t *file_table    = NULL;

    current_lock();
    file_table = current->t_file_table;
    
    ftlock(file_table);
    cred = &file_table->cred;

    /*No. permissions.*/
    if (gid != cred->c_gid && cred->c_gid != 0)
        err = -EPERM;

    else
        cred->c_gid = gid;

    ftunlock(file_table);

    current_unlock();
    return err;
}

int seteuid(uid_t euid) {
    int             err     = 0;
    cred_t          *cred   = NULL;
    file_table_t    *file_table = NULL;

    current_lock();
    file_table = current->t_file_table;
    
    ftlock(file_table);
    cred = &file_table->cred;

    /*No. permissions.*/
    if (euid != cred->c_euid && cred->c_euid != 0)
        err = -EPERM;

    else
        cred->c_euid = euid;

    ftunlock(file_table);

    current_unlock();
    return err;
}

int setegid(gid_t egid) {
    int             err     = 0;
    cred_t          *cred   = NULL;
    file_table_t    *file_table = NULL;

    current_lock();
    file_table = current->t_file_table;
    
    ftlock(file_table);
    cred = &file_table->cred;

    /*No. permissions.*/
    if (egid != cred->c_egid && cred->c_egid != 0)
        err = -EPERM;

    else
        cred->c_egid = egid;

    ftunlock(file_table);

    current_unlock();
    return err;
}

mode_t umask(mode_t cmask) {
    mode_t      omask   = 0;
    file_table_t *ft    = NULL;

    current_lock();
    ft = current->t_file_table;
    ftlock(ft);
    current_unlock();

    omask = ft->cred.c_umask;
    ft->cred.c_umask = cmask & 0777;

    ftunlock(ft);

    return omask;
}

int getcwd(char *buf, size_t size) {
    int ret     = 0;
    file_table_t *ft = NULL;

    if (buf == NULL)
        return -EFAULT;
    
    if (size == 0)
        return -EINVAL;
    
    current_lock();
    ft = current->t_file_table;
    ftlock(ft);

    if (ft->cred.c_cwd == NULL)
        ret = -ENOENT;
    else if (strlen(ft->cred.c_cwd) >= size)
        ret = -ERANGE;
    else
        strncpy(buf, ft->cred.c_cwd, size);

    ftunlock(ft);
    current_unlock();

    return ret;
}

int chdir(const char *path) {
    int err             = 0;
    char *path_dup      = NULL;
    file_table_t *ft    = NULL;

    if ((err = vfs_lookup(path, NULL, O_RDONLY, 0, 0, NULL)))
        return err;

    if ((path_dup = strdup(path)) == NULL)
        return -ENOMEM;

    current_lock();
    ft = current->t_file_table;
    ftlock(ft);

    if (ft->cred.c_cwd)
        kfree(ft->cred.c_cwd);
    ft->cred.c_cwd = path_dup;

    ftunlock(ft);
    current_unlock();

    return 0;
}