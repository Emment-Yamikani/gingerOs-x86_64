#include <bits/errno.h>
#include <fs/dentry.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <fs/inode.h>
#include <lib/string.h>
#include <sys/thread.h>

int vfs_mkdirat(const char *pathname, dentry_t *dir, cred_t *cred, mode_t mode) {
    int         err     = 0;
    vfspath_t   *path   = NULL;

    if (!S_ISDIR(mode)) {
        if (S_IFMT & mode)
            return -EINVAL;
    }
    
    if ((err = vfs_resolve_path(pathname, dir, cred, O_EXCL, &path))) {
        if (err == -ENOENT) {
            // only goto create dir if the traversal reached the last token of the path.
            if (vfspath_islasttoken(path))
                goto creat;
        }
        
        if (path) {
            assert(path->directory, "On error, path has no directory\n");
            dclose(path->directory);
        }
        goto error;
    }

    assert(path->directory == NULL, "On success, path has directory\n");
    assert(path->dentry, "On success, path has no dentry\n");

    err = -EEXIST;
    dclose(path->dentry);
    goto error;
creat:
    ilock(path->directory->d_inode);
    if ((err = imkdir(path->directory->d_inode, path->token, mode & ~S_IFMT))) {
        iunlock(path->directory->d_inode);
        dclose(path->directory);
        goto error;
    }
    iunlock(path->directory->d_inode);

    dclose(path->directory);
    path_free(path);
    return 0;
error:
    if (path)
        path_free(path);
    return err;
}

int vfs_mkdir(const char *path, cred_t *cred, mode_t mode) {
    return vfs_mkdirat(path, NULL, cred, mode);
}