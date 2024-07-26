#include <fs/dentry.h>
#include <fs/fs.h>
#include <fs/inode.h>
#include <lib/string.h>
#include <bits/errno.h>

int vfs_traverse_path(vfspath_t *path, cred_t *cred, int oflags, usize *ptok_i) {
    int         err      = 0;
    usize       tok_i    = 0;
    inode_t     *ip      = NULL;
    dentry_t    *dentry  = NULL;

    if (path == NULL)
        return -EINVAL;

    foreach(token, path->tokenized) {
        dentry          = NULL;
        path->dentry    = NULL;
        path->token     = token;

        ilock(path->directory->d_inode);
        if ((err = icheck_perm(path->directory->d_inode, cred, oflags))) {
            iunlock(path->directory->d_inode);
            // printk("%s:%d: %s() failed to lookup: '%s', err: %d\n", __FILE__, __LINE__, __func__, token, err);
            goto error;
        }

        // Ensure that this path->directory is actually refering to a directory.
        err = IISDIR(path->directory->d_inode) ? 0 : -ENOTDIR;
        iunlock(path->directory->d_inode);

        if (err != 0) {
            // printk("%s:%d: %s() failed to lookup: '%s', err: %d\n", __FILE__, __LINE__, __func__, token, err);
            goto error;
        }

        /**
         * @brief Lookup 'token' in the dentry's cache
         * of children.
         * unlock 'path->directory' if err != -ENOENT*/
        if ((err = dlookup(path->directory, token, &dentry)) == -ENOENT) {
            // if current path->directory has an inode follow the fs-specific lookup.
            if (path->directory->d_inode != NULL) {
                foreach(token, &path->tokenized[tok_i]) {
                    dentry          = NULL;
                    path->dentry    = NULL;
                    path->token     = token;

                    ilock(path->directory->d_inode);
                    if ((err = ilookup(path->directory->d_inode, token, &ip))) {
                        iunlock(path->directory->d_inode);
                        // printk("%s:%d: %s() failed to lookup: '%s', err: %d\n", __FILE__, __LINE__, __func__, token, err);
                        goto error;
                    }
                    iunlock(path->directory->d_inode);

                    // found the inode.
                    if ((err = icheck_perm(ip, cred, oflags))) {
                        irelease(ip);
                        // printk("%s:%d: %s() failed to lookup: '%s', err: %d\n", __FILE__, __LINE__, __func__, token, err);
                        goto error;
                    }

                    // add dentry alias to the new inode.
                    if ((err = imkalias(ip, token, &dentry))) {
                        irelease(ip);
                        // printk("%s:%d: %s() failed to lookup: '%s', err: %d\n", __FILE__, __LINE__, __func__, token, err);
                        goto error;
                    }

                    // bind the new dentry to the directory tree.
                    if ((err = dbind(path->directory, dentry))) {
                        irelease(ip);
                        // printk("%s:%d: %s() failed to lookup: '%s', err: %d\n", __FILE__, __LINE__, __func__, token, err);
                        goto error;
                    }

                    err = IISDIR(ip) ? 0: -ENOTDIR;
                    irelease(ip);

                    path->dentry = dentry;
                    // reached the end of the path?
                    if (!compare_strings(token, path->lasttoken)) {
                        /**
                         * we've reached the end of the path.
                         * path fully traversed but does not refer to a directory.
                         * user specified '/' at the end of the pathname,
                         * therefore this is an error.*/
                        if (vfspath_isdir(path) && err == -ENOTDIR) {
                            goto error;
                        }
                    } else { // we've not reached the end of the path.
                        if (err == - ENOTDIR) {
                            /** ERROR: the next token cannot be traversed
                             * because it is not a directory.*/
                            // printk("%s:%d: %s() failed to lookup: '%s', err: %d\n", __FILE__, __LINE__, __func__, token, err);
                            goto error;
                        }

                        dclose(path->directory);
                        // next token is a dir and can be followed.
                        tok_i++;
                        path->directory     = dentry;
                    }
                }

                goto done;
            }
        } else if (err != 0) {
            // printk("%s:%d: %s() failed to lookup: '%s', err: %d\n", __FILE__, __LINE__, __func__, token, err);
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
                // printk("%s:%d: %s() failed to lookup: '%s', err: %d\n", __FILE__, __LINE__, __func__, token, err);
                goto error;
            }
            err = IISDIR(dentry->d_inode) ? 0: -ENOTDIR;
            iunlock(dentry->d_inode);
        }

        path->dentry = dentry;
        // reached the end of the path?
        if (!compare_strings(token, path->lasttoken)) {
            /**
             * we've reached the end of the path.
             * path fully traversed but does not refer to a directory.
             * user specified '/' at the end of the pathname,
             * therefore this is an error.*/
            if (vfspath_isdir(path) && err == -ENOTDIR) {
                goto error;
            }
        } else { // we've not reached the end of the path.
            if (err == - ENOTDIR) {
                /** ERROR: the next token cannot be traversed
                 * because it is not a directory.*/
                goto error;
            }

            dclose(path->directory);
            // next token is a dir and can be followed.
            tok_i++;
            path->directory     = dentry;
        }
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