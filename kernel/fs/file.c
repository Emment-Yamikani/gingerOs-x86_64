#include <fs/file.h>
#include <fs/dentry.h>
#include <mm/kalloc.h>
#include <bits/errno.h>
#include <sys/thread.h>
#include <dev/dev.h>
#include <fs/fs.h>

int fctx_alloc(file_ctx_t **ret) {
    int         err         = 0;
    file_ctx_t  *fctx       = NULL;
    dentry_t    *cwdir      = NULL;
    dentry_t    *rootdir    = NULL;

    if (ret == NULL)
        return -EINVAL;
    
    if (NULL == (fctx = (file_ctx_t *)kmalloc(sizeof *fctx)))
        return -ENOMEM;

    if ((err = vfs_lookup("/", NULL, O_EXCL, &cwdir)))
        goto error;

    rootdir = ddup(cwdir);
    dunlock(cwdir);

    memset(fctx, 0, sizeof *fctx);

    fctx->fc_fmax   = NFILE;
    fctx->fc_cwd    = cwdir;
    fctx->fc_root   = rootdir;
    fctx->fc_lock   = SPINLOCK_INIT();

    *ret = fctx;
    return 0;
error:
    if (fctx)
        kfree(fctx);
    return err;
}

void fctx_free(file_ctx_t *fctx) {
    file_t *file = NULL;

    assert(fctx, "No file context\n");

    if (!fctx_islocked(fctx))
        fctx_lock(fctx);
    
    dclose(fctx->fc_cwd);
    dclose(fctx->fc_root);

    for (int i = 0; i < fctx->fc_nfile; ++i) {
        file = fctx->fc_files[i];
        if (file) {
            flock(file);
            fclose(file);
            
        }
    }

    kfree(fctx->fc_files);
    fctx_unlock(fctx);
    kfree(fctx);
}

int     falloc(file_t **pfp) {
    file_t *file = NULL;

    if ((file = kcalloc(1, sizeof *file)) == NULL)
        return -ENOMEM;
    
    file->f_refcnt = 1;
    file->f_lock = SPINLOCK_INIT();
    flock(file);    
    *pfp = file;
    return 0;
}

void    fdestroy(file_t *file) {
    if (file == NULL)
        return;
    if (!fislocked(file))
        flock(file);
    funlock(file);
    kfree(file);
}

int     fdup(file_t *file) {
    if (file == NULL)
        return -EINVAL;
    fassert_locked(file);
    file->f_refcnt++;
    return 0;
}

int     fput(file_t *file) {
    if (file == NULL)
        return -EINVAL;
    fassert_locked(file);
    file->f_refcnt--;
    return 0;
}

int     feof(file_t *file) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(file);

    if (file->fops != NULL && file->fops->feof != NULL) {
        return file->fops->feof(file);
    }

    // Generic write operation
    if (file->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(file->f_dentry); // Lock dentry
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(file->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }
    
    err = file->f_off >= igetsize(inode);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     fclose(file_t *file) {
    int err = 0;
    fassert_locked(file);

    if (file->fops != NULL && file->fops->fclose != NULL) {
        return file->fops->fclose(file);
    }

    if ((err = fput(file)))
        return err;

    if (file->f_refcnt <= 0) {
        dlock(file->f_dentry);
        dclose(file->f_dentry);
        fdestroy(file);
        goto done;
    }

    funlock(file);
done:
    return 0;
}

int     fsync(file_t *file) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(file);

    if (file->fops != NULL && file->fops->fsync != NULL) {
        return file->fops->fsync(file);
    }

    // Generic write operation
    if (file->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(file->f_dentry); // Lock dentry
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(file->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }

    err = isync(inode);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     funlink(file_t *file) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(file);

    if (file->fops != NULL && file->fops->funlink != NULL) {
        return file->fops->funlink(file);
    }


    // Generic write operation
    if (file->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(file->f_dentry); // Lock dentry
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(file->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }
    
    err = iunlink(inode);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     fgetattr(file_t *file, void *attr) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(file);

    if (file->fops != NULL && file->fops->fgetattr != NULL) {
        return file->fops->fgetattr(file, attr);
    }

    // Generic write operation
    if (file->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(file->f_dentry); // Lock dentry
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(file->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }
    
    err = igetattr(inode, attr);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     fsetattr(file_t *file, void *attr) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(file);

    if (file->fops != NULL && file->fops->fsetattr != NULL) {
        return file->fops->fsetattr(file, attr);
    }

    // Generic write operation
    if (file->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(file->f_dentry); // Lock dentry
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(file->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }
    
    err = isetattr(inode, attr);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     ftruncate(file_t *file, off_t length) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(file);

    if (file->fops != NULL && file->fops->ftruncate != NULL) {
        return file->fops->ftruncate(file, length);
    }

    // Generic write operation
    if (file->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(file->f_dentry); // Lock dentry
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(file->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }

    err = itruncate(inode);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     ffcntl(file_t *file, int cmd, void *argp) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(file);

    if (file->fops != NULL && file->fops->ffcntl != NULL) {
        return file->fops->ffcntl(file, cmd, argp);
    }

    // Generic write operation
    if (file->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(file->f_dentry); // Lock dentry
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(file->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }
    
    err = ifcntl(inode, cmd, argp);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     fioctl(file_t *file, int req, void *argp) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(file);

    if (file->fops != NULL && file->fops->fioctl != NULL) {
        return file->fops->fioctl(file, req, argp);
    }

    // Generic write operation
    if (file->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(file->f_dentry); // Lock dentry
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(file->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }
    
    err = iioctl(inode, req, argp);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

off_t   flseek(file_t *file, off_t off, int whence) {
    inode_t *inode = NULL;

    fassert_locked(file);

    if (file->fops != NULL && file->fops->flseek != NULL) {
        return file->fops->flseek(file, off, whence);
    }

    // Generic write operation
    if (file->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(file->f_dentry); // Lock dentry
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(file->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }

    switch (whence) {
        case 0: /* SEEK_SET */
            file->f_off = off;
            break;
        case 1: /* SEEK_CUR */
            file->f_off += off;
            break;
        case 2: /* SEEK_END */
            file->f_off = inode->i_size + off;
            break;
    }
    off = file->f_off;

    iputcnt(inode);
    iunlock(inode);
    return off;
}

ssize_t fread(file_t *file, void *buf, size_t size) {
    ssize_t retval = 0;
    inode_t *inode = NULL;

    fassert_locked(file); // Ensure file is locked

    // If file-specific read operation exists, use it
    if (file->fops != NULL && file->fops->fread != NULL) {
        return file->fops->fread(file, buf, size);
    }

    // Generic read operation
    if (file->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(file->f_dentry); // Lock dentry
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(file->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }

    while (1) {
        // Attempt to read data
        retval = iread(inode, file->f_off, buf, size);

        if (retval > 0) { // Data successfully read
            file->f_off += retval;
            if (inode->i_writers) {
                cond_broadcast(inode->i_writers); // Notify writers
            }
            break;
        } else if (retval == 0) { // End-of-file reached
            break;
        } else if (retval < 0) { // Error occurred during read
            break;
        } else if (file->f_oflags & O_NONBLOCK) { // Non-blocking mode
            retval = -EAGAIN; // Operation would block
            break;
        } else { // Blocking mode, no data available
            if (inode->i_readers) {
                cond_wait(inode->i_readers); // Wait for data to be written
            }
        }
    }

    iputcnt(inode); // Decrement reference count
    iunlock(inode); // Unlock inode
    return retval;
}

ssize_t fwrite(file_t *file, void *buf, size_t size) {
    ssize_t retval = 0;
    inode_t *inode = NULL;

    fassert_locked(file); // Ensure file is locked

    // If file-specific write operation exists, use it
    if (file->fops != NULL && file->fops->fwrite != NULL) {
        return file->fops->fwrite(file, buf, size);
    }

    // Generic write operation
    if (file->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(file->f_dentry); // Lock dentry
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(file->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }

    while (1) {
        if (file->f_oflags & O_NONBLOCK) { // Non-blocking mode
            if ((file->f_off + size) < igetsize(inode)) { // Can write
                retval = iwrite(inode, file->f_off, buf, size);
                if (retval >= 0) {
                    file->f_off += retval;
                    if (inode->i_readers) {
                        cond_broadcast(inode->i_readers); // Notify readers
                    }
                } else {
                    break; // Error during write
                }
            } else {
                retval = -EAGAIN; // Operation would block
                break;
            }
        } else { // Blocking mode
            retval = 0;
            ssize_t written = 0;
            size_t remaining = size;

            while (remaining > 0) {
                written = iwrite(inode, file->f_off, buf, remaining);
                if (written < 0) {
                    if (retval == 0) { // No data has been written yet
                        retval = written; // Return the error code
                    }
                    break;
                }

                retval += written;
                remaining -= written;
                file->f_off += written;
                buf = (char *)buf + written;

                if (remaining > 0) {
                    if (inode->i_writers) {
                        cond_wait(inode->i_writers); // Wait for space to write
                    } else {
                        break; // No wait queue to sleep on, exit
                    }
                }

                if (inode->i_readers) {
                    cond_broadcast(inode->i_readers); // Notify readers
                }
            }

            break;
        }
    }

    iputcnt(inode); // Decrement reference count
    iunlock(inode); // Unlock inode
    return retval;
}

int     fcreate(file_t *dir, const char *pathname, mode_t mode) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(dir);

    if (dir->fops != NULL && dir->fops->fcreate != NULL) {
        return dir->fops->fcreate(dir, pathname, mode);
    }

    // Generic write operation
    if (dir->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(dir->f_dentry); // Lock dentry
    if ((inode = dir->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(dir->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }
    
    err = icreate(inode, pathname, mode);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     fmkdirat(file_t *dir, const char *pathname, mode_t mode) {
    int err = 0;
    inode_t *inode = NULL;
    __unused char *path = NULL, *filename = NULL;
    
    fassert_locked(dir);
    
    if ((err = path_get_lasttoken(pathname, &filename)))
        return err;

    if (dir->fops != NULL && dir->fops->fmkdirat != NULL) {
        return dir->fops->fmkdirat(dir, filename, mode);
    }

    // Generic write operation
    if (dir->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(dir->f_dentry); // Lock dentry
    if ((inode = dir->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(dir->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }
    
    err = imkdir(inode, filename, mode);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

ssize_t freaddir(file_t *dir, off_t off, void *buf, size_t count) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(dir);

    if (dir->fops != NULL && dir->fops->freaddir != NULL) {
        return dir->fops->freaddir(dir, off, buf, count);
    }

    // Generic write operation
    if (dir->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(dir->f_dentry); // Lock dentry
    if ((inode = dir->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(dir->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }
    
    err = ireaddir(inode, off, buf, count);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     flinkat(file_t *dir, const char *oldname, const char *newname) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(dir);

    if (dir->fops != NULL && dir->fops->flinkat != NULL) {
        return dir->fops->flinkat(dir, oldname, newname);
    }

    // Generic write operation
    if (dir->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(dir->f_dentry); // Lock dentry
    if ((inode = dir->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(dir->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }
    
    err = ilink(oldname, inode, newname);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     fmknodat(file_t *dir, const char *pathname, mode_t mode, int devid) {
    int err = 0;
    inode_t *inode = NULL;
    char *filename = NULL;

    if ((err = path_get_lasttoken(pathname, &filename)))
        return err;

    fassert_locked(dir);

    if (dir->fops != NULL && dir->fops->fmknodat != NULL) {
        return dir->fops->fmknodat(dir, filename, mode, devid);
    }
    
    // Generic write operation
    if (dir->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(dir->f_dentry); // Lock dentry
    if ((inode = dir->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(dir->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }
    
    err = imknod(inode, filename, mode, devid);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int fmmap(file_t *file, vmr_t *region) {
    int     err     = 0;
    inode_t *inode  = NULL;

    if (file == NULL || region == NULL)
        return -EINVAL;
    
    fassert_locked(file);

    if (__vmr_read(region)) {
        if (((file->f_oflags & O_ACCMODE) != O_RDONLY)
            && ((file->f_oflags & O_ACCMODE) != O_RDWR))
            return -EACCES;
    }

    if (__vmr_write(region)) {
        if (((file->f_oflags & O_ACCMODE) != O_WRONLY)
            && ((file->f_oflags & O_ACCMODE) != O_RDWR))
            return -EACCES;
    }

    if (__vmr_exec(region)) {
        if (!(file->f_oflags & O_EXCL))
            return -EACCES;
    }

    if (file->fops != NULL && file->fops->fmmap != NULL) {
        return file->fops->fmmap(file, region);
    }

    // Generic write operation
    if (file->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(file->f_dentry); // Lock dentry
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(file->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }
    
    if ((IISDEV(inode) == 0) && (IISREG(inode) == 0)) {
        irelease(inode);
        return -ENODEV;
    }
    
    region->file = inode;

    if (IISDEV(inode)) {
        if ((err = kdev_mmap(IDEVID(inode), region))) {
            irelease(inode);
            return err;
        }
    }

    iunlock(inode);
    return 0;
}

int file_stat(file_t *file, struct stat *buf) {
    int     err     = 0;
    inode_t *inode  = NULL;

    fassert_locked(file);

    if (file->fops != NULL && file->fops->fstat != NULL) {
        return file->fops->fstat(file, buf);
    }

    // Generic write operation
    if (file->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(file->f_dentry); // Lock dentry
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(file->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }

    err = istat(inode, buf);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int file_chown(file_t *file, uid_t owner, gid_t group) {
    int     err     = 0;
    inode_t *inode  = NULL;

    fassert_locked(file);

    if (file->fops != NULL && file->fops->fchown != NULL) {
        return file->fops->fchown(file, owner,group);
    }

    // Generic write operation
    if (file->f_dentry == NULL) {
        return -ENOENT; // No such file or directory
    }

    dlock(file->f_dentry); // Lock dentry
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode); // Lock inode
        idupcnt(inode); // Increment reference count
    }
    dunlock(file->f_dentry); // Unlock dentry

    if (inode == NULL) {
        return -ENOENT; // No such file or directory
    }

    err = ichown(inode, owner, group);
    iputcnt(inode);
    iunlock(inode);
    return err;
}