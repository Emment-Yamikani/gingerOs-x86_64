#include <fs/dentry.h>
#include <fs/fs.h>
#include <fs/inode.h>
#include <lib/string.h>
#include <bits/errno.h>

int vfs_traverse_path(dentry_t *dir, cred_t *cred, int oflags, vfspath_t *path, usize *ptok_i) {
    int     err      = 0;
    usize   tok_i    = 0;
    dentry_t *dentry = NULL;

    if (dir == NULL || path == NULL)
        return -EINVAL;

    foreach(token, path->tokenized) {
        dentry          = NULL;
        path->dentry    = NULL;
        path->directory = dir;

        ilock(dir->d_inode);
        if ((err = icheck_perm(dir->d_inode, cred, oflags))) {
            iunlock(dir->d_inode);
            printk("%s:%d: %s() failed to lookup: '%s', err: %d\n", __FILE__, __LINE__, __func__, token, err);
            goto error;
        }

        // Ensure that this dir is actually refering to a directory.
        err = IISDIR(dir->d_inode) ? 0 : -ENOTDIR;
        iunlock(dir->d_inode);
        
        if (err != 0) {
            printk("%s:%d: %s() failed to lookup: '%s', err: %d\n", __FILE__, __LINE__, __func__, token, err);
            goto error;
        }

        /**
         * @brief Lookup 'token' in the dentry's cache
         * of children.
         * unlock 'dir' if err != -ENOENT*/
        if ((err = dlookup(dir, token, &dentry)) != -ENOENT) {
            dclose(dir);
        }

        if (err != 0) {
            printk("%s:%d: %s() failed to lookup: '%s', err: %d\n", __FILE__, __LINE__, __func__, token, err);
            goto error;
        }

        /**
         * @brief if dentry has an inode
         * check if we have the neccessary permission
         * to access the inode.*/
        if (dentry->d_inode) {
            ilock(dentry->d_inode);
            if ((err = icheck_perm(dentry->d_inode, cred, oflags))) {
                iunlock(dentry->d_inode);
                dclose(dentry);
                printk("%s:%d: %s() failed to lookup: '%s', err: %d\n", __FILE__, __LINE__, __func__, token, err);
                goto error;
            }
            iunlock(dentry->d_inode);
        }

        // reached the last token in the pathname
        if (!compare_strings(token, path->lasttoken)) {
            if (dentry->d_inode) {
                ilock(dentry->d_inode);
                // if the pathname had '/' at the end check if inode is a directory.
                if (vfspath_isdir(path) && !IISDIR(dentry->d_inode))
                    err = -ENOTDIR;
                iunlock(dentry->d_inode);

                 // fail if path specified a directory but actual file isn't.
                if (err != 0) {
                    dclose(dentry);
                    printk("%s:%d: %s() failed to lookup: '%s', err: %d\n", __FILE__, __LINE__, __func__, token, err);
                    goto error;
                }
            }

            path->dentry    = dentry;
            goto done;
        }

        // Not reached the last token.
        tok_i++;
        dir = dentry;
    }

done:
    if (ptok_i)
        *ptok_i = tok_i;
    return 0;
error:
    if (ptok_i)
        *ptok_i = tok_i;
    return err;
}