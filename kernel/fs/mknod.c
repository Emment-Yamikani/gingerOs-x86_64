#include <bits/errno.h>
#include <dev/dev.h>
#include <fs/dentry.h>
#include <fs/inode.h>
#include <sys/thread.h>
#include <lib/string.h>
#include <fs/stat.h>

int vfs_mknodat(const char *pathname, dentry_t *dir,
                    cred_t *cred, mode_t mode, devid_t dev) {
    int         err   = 0;
    vfspath_t   *path = NULL;
    
    // Validate the mode
    if ((mode & S_IFMT) == 0) {
        mode |= S_IFREG; // Default to regular file
    }
    
    if (!S_ISCHR(mode) && !S_ISBLK(mode) &&
        !S_ISFIFO(mode) && !S_ISSOCK(mode) && !S_ISREG(mode)) {
        return -EINVAL; // Invalid argument
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
    dclose(path->directory);
    goto error;
creat:
    ilock(path->directory->d_inode);
    if ((err = imknod(path->directory->d_inode, path->token, mode, dev))) {
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

int vfs_mknod(const char *pathname, cred_t *cred, mode_t mode, devid_t dev) {
    return vfs_mknodat(pathname, NULL, cred, mode, dev);
}