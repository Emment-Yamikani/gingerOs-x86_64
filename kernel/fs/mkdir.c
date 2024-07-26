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

    if ((err = parse_path(pathname, NULL, 0, &path)))
        return err;

    if (dir == NULL) {
        if ((dir = vfs_getdroot()) == NULL) {
            err = -EINVAL;
            goto error;
        }
    }

    path->directory = dir;
    if ((err = vfs_traverse_path(path, cred, O_EXCL, NULL)) != -ENOENT) {
        goto error;
    } else if (err == 0) {
        err = -EEXIST;
        goto error;
    }
    
    // doesn't exist.

    ilock(path->directory->d_inode);
    if ((err = imkdir(path->directory->d_inode, path->token, mode & ~S_IFMT))) {
        iunlock(path->directory->d_inode);
        goto error;
    }
    iunlock(path->directory->d_inode);

    if (path->dentry)
        dclose(path->dentry);
    if (path->directory)
        dclose(path->directory);

    return 0;
error:
    if (path->dentry)
        dclose(path->dentry);
    if (path->directory)
        dclose(path->directory);
    if (path)
        path_free(path);
    return err;
}