#include <bits/errno.h>
#include <fs/fs.h>
#include <fs/dentry.h>
#include <fs/devfs.h>
// #include <fs/ramfs.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <printk.h>
// #include <fs/pipefs.h>
#include <fs/tmpfs.h>

int vfs_mountat(const char *__src, const char *__target,
                const char *__type, uint32_t __mount_flags,
                const void *__data, inode_t *__inode, uio_t *__uio) {
    int err = -EINVAL;
    struct filesystem *fs = NULL;
    char **target_tokens = NULL, *src_tokens = NULL;
    dentry_t *parent_dentry = NULL, *child_dentry = NULL;
    char *cwd = NULL, *abs_path_src = NULL, *abs_path_target = NULL;

    if (__uio && (__uio->u_uid && __uio->u_gid))
        return -EPERM;

    if (!__uio)
        cwd = "/";
    else if (!__uio->u_cwd)
        cwd = "/";
    else
        cwd = __uio->u_cwd;

    if ((err = parse_path((char *)__target, cwd, &abs_path_target, NULL, NULL)))
        goto error;

    if (__mount_flags & MS_REMOUNT) {
        goto error;
    }

    if (__mount_flags & MS_BIND) {
        if ((err = vfs_lookup(__target, __uio, O_RDWR, 0777, 0, NULL, &parent_dentry)))
            goto error;
        
        if ((err = dentry_iset(parent_dentry, __inode, 1)))
            goto error;
        
        goto error; // TODO: handle filesystem specific bind mount.
    }

    if (__mount_flags & MS_MOVE) {
        goto error;
    }

    if (MS_NONE(__mount_flags)) {
        goto done;
    }

    if (__type) {
        goto done;
    }

done:
    return 0;
error:
    return err;
}