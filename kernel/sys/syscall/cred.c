#include <fs/fs.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <sys/sysprot.h>
#include <sys/thread.h>
#include <mm/kalloc.h>
#include <fs/cred.h>

int cred_alloc(cred_t **ret) {
    cred_t *cred = NULL;

    if (ret == NULL)
        return -EINVAL;
    
    if (NULL == (cred = (cred_t *)kmalloc(sizeof *cred)))
        return -ENOMEM;
    
    memset(cred, 0, sizeof *cred);
    *cred = CRED_DEFAULT();

    *ret = cred;

    return 0;
}

void cred_free(cred_t *cred) {
    assert(cred, "No credentials.");

    if (!cred_islocked(cred))
        cred_lock(cred);
    
    cred_unlock(cred);
    kfree(cred);
}

int cred_copy(cred_t *dst, cred_t *src) {
    if (dst == NULL || src == NULL)
        return -EINVAL;
    
    cred_assert_locked(dst);
    cred_assert_locked(src);

    dst->c_uid      = src->c_uid;
    dst->c_gid      = src->c_gid;
    dst->c_euid     = src->c_euid;
    dst->c_egid     = src->c_egid;
    dst->c_suid     = src->c_suid;
    dst->c_sgid     = src->c_sgid;
    dst->c_umask    = src->c_umask;

    return 0;
}

uid_t getuid(void) {
    uid_t   uid     = 0;
    cred_t  *cred   = NULL;

    current_lock();
    cred = current->t_cred;

    cred_lock(cred);
    uid = cred->c_uid;
    cred_unlock(cred);

    current_unlock();
    return uid;
}

gid_t getgid(void) {
    gid_t            gid    = 0;
    cred_t          *cred   = NULL;

    current_lock();
    cred = current->t_cred;

    cred_lock(cred);
    gid = cred->c_gid;
    cred_unlock(cred);

    current_unlock();
    return gid;
}

uid_t geteuid(void) {
    uid_t uid = 0;
    cred_t *cred = NULL;

    current_lock();
    
    cred = current->t_cred;
    cred_lock(cred);
    uid = cred->c_euid;
    cred_unlock(cred);

    current_unlock();
    return uid;
}

gid_t getegid(void) {
    gid_t   gid     = 0;
    cred_t  *cred   = NULL;

    current_lock();
    
    cred = current->t_cred;

    cred_lock(cred);
    gid = cred->c_egid;
    cred_unlock(cred);

    current_unlock();
    return gid;
}

int setuid(uid_t uid) {
    int             err     = 0;
    cred_t          *cred   = NULL;

    current_lock();

    cred = current->t_cred;

    cred_lock(cred);

    /*No. permissions.*/
    if (uid != cred->c_uid && cred->c_uid != 0)
        err = -EPERM;

    else
        cred->c_uid = uid;

    cred_unlock(cred);

    current_unlock();
    return err;
}

int setgid(gid_t gid) {
    int          err    = 0;
    cred_t       *cred  = NULL;

    current_lock();
    
    cred = current->t_cred;
    cred_lock(cred);

    /*No. permissions.*/
    if (gid != cred->c_gid && cred->c_gid != 0)
        err = -EPERM;

    else
        cred->c_gid = gid;

    cred_unlock(cred);

    current_unlock();
    return err;
}

int seteuid(uid_t euid) {
    int             err     = 0;
    cred_t          *cred   = NULL;

    current_lock();
    
    cred = current->t_cred;
    cred_lock(cred);

    /*No. permissions.*/
    if (euid != cred->c_euid && cred->c_euid != 0)
        err = -EPERM;

    else
        cred->c_euid = euid;

    cred_unlock(cred);

    current_unlock();
    return err;
}

int setegid(gid_t egid) {
    int             err     = 0;
    cred_t          *cred   = NULL;

    current_lock();
    
    cred = current->t_cred;
    cred_lock(cred);

    /*No. permissions.*/
    if (egid != cred->c_egid && cred->c_egid != 0)
        err = -EPERM;

    else
        cred->c_egid = egid;

    cred_unlock(cred);

    current_unlock();
    return err;
}

mode_t umask(mode_t cmask) {
    mode_t      omask   = 0;
    cred_t      *cred = NULL;

    current_lock();
    cred = current->t_cred;

    cred_lock(cred);
    omask = cred->c_umask;
    cred->c_umask = cmask & 0777;
    cred_unlock(cred);

    current_unlock();

    return omask;
}

int getcwd(char *buf, size_t size) {
    int ret     = 0;
    char *path  = NULL;
    size_t      len = 0;
    file_ctx_t  *file_ctx = NULL;

    if (buf == NULL)
        return -EFAULT;
    
    if (size == 0)
        return -EINVAL;
    
    current_lock();
    
    file_ctx = current->t_fctx;
    fctx_lock(file_ctx);
    ret = dretrieve_path(file_ctx->fc_cwd, &path, &len);
    fctx_unlock(file_ctx);

    current_unlock();

    if (ret == 0) {
        if (len >= size)
            ret = -ERANGE;
        else
            safestrncpy(buf, path, len);
        
        kfree(path);
    }

    return ret;
}

int chdir(const char *path) {
    int         err         = 0;
    dentry_t     *newcwd    = NULL;
    file_ctx_t *file_ctx  = NULL;

    if ((err = vfs_lookup(path, NULL, O_RDONLY, 0, 0, &newcwd)))
        return err;

    current_lock();
    file_ctx = current->t_fctx;
    fctx_lock(file_ctx);

    dclose(file_ctx->fc_cwd);
    file_ctx->fc_cwd = newcwd;
    dunlock(newcwd);

    fctx_unlock(file_ctx);
    current_unlock();

    return 0;
}