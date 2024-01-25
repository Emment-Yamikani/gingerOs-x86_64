#include <fs/file.h>
#include <fs/dentry.h>
#include <mm/kalloc.h>
#include <bits/errno.h>
#include <sys/thread.h>
#include <dev/dev.h>
#include <fs/fs.h>

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

    if (file->fops == NULL || file->fops->feof == NULL)
        goto generic;

    return file->fops->feof(file);
generic:
    if (file->f_dentry == NULL)
        return -ENOENT;

    dlock(file->f_dentry);
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode);
        idupcnt(inode);
    }
    dunlock(file->f_dentry);

    if (inode == NULL)
        return -ENOENT;
    
    err = file->f_off >= igetsize(inode);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     fclose(file_t *file) {
    int err = 0;
    fassert_locked(file);

    if (file->fops == NULL || file->fops->fclose == NULL)
        goto generic;

    return file->fops->fclose(file);
generic:
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

    if (file->fops == NULL || file->fops->fsync == NULL)
        goto generic;

    return file->fops->fsync(file);
generic:
    if (file->f_dentry == NULL)
        return -ENOENT;

    dlock(file->f_dentry);
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode);
        idupcnt(inode);
    }
    dunlock(file->f_dentry);

    if (inode == NULL)
        return -ENOENT;

    err = isync(inode);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     funlink(file_t *file) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(file);

    if (file->fops == NULL || file->fops->funlink == NULL)
        goto generic;

    return file->fops->funlink(file);

generic:
    if (file->f_dentry == NULL)
        return -ENOENT;

    dlock(file->f_dentry);
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode);
        idupcnt(inode);
    }
    dunlock(file->f_dentry);

    if (inode == NULL)
        return -ENOENT;
    
    err = iunlink(inode);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     fgetattr(file_t *file, void *attr) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(file);

    if (file->fops == NULL || file->fops->fgetattr == NULL)
        goto generic;

    return file->fops->fgetattr(file, attr);

generic:
    if (file->f_dentry == NULL)
        return -ENOENT;

    dlock(file->f_dentry);
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode);
        idupcnt(inode);
    }
    dunlock(file->f_dentry);

    if (inode == NULL)
        return -ENOENT;
    
    err = igetattr(inode, attr);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     fsetattr(file_t *file, void *attr) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(file);

    if (file->fops == NULL || file->fops->fsetattr == NULL)
        goto generic;

    return file->fops->fsetattr(file, attr);

generic:
    if (file->f_dentry == NULL)
        return -ENOENT;

    dlock(file->f_dentry);
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode);
        idupcnt(inode);
    }
    dunlock(file->f_dentry);

    if (inode == NULL)
        return -ENOENT;
    
    err = isetattr(inode, attr);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     ftruncate(file_t *file, off_t length __unused) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(file);

    if (file->fops == NULL || file->fops->ftruncate == NULL)
        goto generic;

    return file->fops->ftruncate(file, length);

generic:
    if (file->f_dentry == NULL)
        return -ENOENT;

    dlock(file->f_dentry);
    if ((inode = file->f_dentry->d_inode))
    {
        ilock(inode);
        idupcnt(inode);
    }
    dunlock(file->f_dentry);

    if (inode == NULL)
        return -ENOENT;

    err = itruncate(inode);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     ffcntl(file_t *file, int cmd, void *argp) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(file);

    if (file->fops == NULL || file->fops->ffcntl == NULL)
        goto generic;

    return file->fops->ffcntl(file, cmd, argp);

generic:
    if (file->f_dentry == NULL)
        return -ENOENT;

    dlock(file->f_dentry);
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode);
        idupcnt(inode);
    }
    dunlock(file->f_dentry);

    if (inode == NULL)
        return -ENOENT;
    
    err = ifcntl(inode, cmd, argp);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     fioctl(file_t *file, int req, void *argp) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(file);

    if (file->fops == NULL || file->fops->fioctl == NULL)
        goto generic;

    return file->fops->fioctl(file, req, argp);

generic:
    if (file->f_dentry == NULL)
        return -ENOENT;

    dlock(file->f_dentry);
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode);
        idupcnt(inode);
    }
    dunlock(file->f_dentry);

    if (inode == NULL)
        return -ENOENT;
    
    err = iioctl(inode, req, argp);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

off_t   flseek(file_t *file, off_t off, int whence) {
    inode_t *inode = NULL;

    fassert_locked(file);

    if (file->fops == NULL || file->fops->flseek == NULL)
        goto generic;

    return file->fops->flseek(file, off, whence);

generic:
    if (file->f_dentry == NULL)
        return -ENOENT;

    dlock(file->f_dentry);
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode);
        idupcnt(inode);
    }
    dunlock(file->f_dentry);

    if (inode == NULL)
        return -ENOENT;

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

    fassert_locked(file);

    if (file->fops == NULL || file->fops->fread == NULL)
        goto generic;

    return file->fops->fread(file, buf, size);

generic:
    if (file->f_dentry == NULL)
        return -ENOENT;

    dlock(file->f_dentry);
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode);
        idupcnt(inode);
    }
    dunlock(file->f_dentry);

    if (inode == NULL)
        return -ENOENT;

    loop() {
        if ((retval = iread(inode, file->f_off, buf, size)) > 0) {
            file->f_off += retval;
            // wake up writers.
            if (inode->i_writers)
                cond_broadcast(inode->i_writers);
            break;
        } else if ((file->f_off >= igetsize(inode))) { // has reached end-of-file
            retval = 0;
            break;
        }
        else if (retval < 0)
            break;
        else if (file->f_oflags & O_NONBLOCK) {
            retval = -EAGAIN;
            break;
        }
        else {
            // sleep on readers' queue waiting for for writers.
            if (inode->i_readers)
                cond_wait(inode->i_readers);
        }
    }    

    iputcnt(inode);
    iunlock(inode);
    return retval;
}

ssize_t fwrite(file_t *file, void *buf, size_t size) {
    ssize_t retval = 0;
    inode_t *inode = NULL;

    fassert_locked(file);

    if (file->fops == NULL || file->fops->fwrite == NULL)
        goto generic;

    return file->fops->fwrite(file, buf, size);
generic:
    if (file->f_dentry == NULL)
        return -ENOENT;

    dlock(file->f_dentry);
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode);
        idupcnt(inode);
    }
    dunlock(file->f_dentry);

    if (inode == NULL)
        return -ENOENT;
    
    loop() {
        if (file->f_oflags & O_NONBLOCK) {
            if (((file->f_off + size) < igetsize(inode))) { // can write.
                if ((retval = iwrite(inode, file->f_off, buf, size)) >= 0) {
                    file->f_off += retval;
                    // wake up any waiting readers.
                    if (inode->i_readers)
                        cond_broadcast(inode->i_readers);
                } else if (retval < 0)
                    break;
            } else {
                retval = -EAGAIN;
                break;
            }
        } else {
            retval = size;
            ssize_t written = 0;

            while (size) {
                if ((written = iwrite(inode, file->f_off, buf, size)) < 0) {
                    retval = written;
                    break;
                }
                size -= written;
                if ((size == 0) || (file->f_off >= igetsize(inode)))
                    break;
                
                // wakeup any readers.
                if (inode->i_readers)
                    cond_broadcast(inode->i_readers);
                // sleep on writters queue.
                if (inode->i_writers)
                    cond_wait(inode->i_writers);
            }

            retval -= size;
            file->f_off += retval;
            // wakeup any readers.
            if (inode->i_readers)
                cond_broadcast(inode->i_readers);
            break;
        }

    }
    iputcnt(inode);
    iunlock(inode);
    return retval;
}

int     fcreate(file_t *dir, const char *pathname, mode_t mode) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(dir);

    if (dir->fops == NULL || dir->fops->fcreate == NULL)
        goto generic;

    return dir->fops->fcreate(dir, pathname, mode);

generic:
    if (dir->f_dentry == NULL)
        return -ENOENT;

    dlock(dir->f_dentry);
    if ((inode = dir->f_dentry->d_inode)) {
        ilock(inode);
        idupcnt(inode);
    }
    dunlock(dir->f_dentry);

    if (inode == NULL)
        return -ENOENT;
    
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

    if (dir->fops == NULL || dir->fops->fmkdirat == NULL)
        goto generic;

    return dir->fops->fmkdirat(dir, filename, mode);

generic:
    if (dir->f_dentry == NULL)
        return -ENOENT;

    dlock(dir->f_dentry);
    if ((inode = dir->f_dentry->d_inode)) {
        ilock(inode);
        idupcnt(inode);
    }
    dunlock(dir->f_dentry);

    if (inode == NULL)
        return -ENOENT;
    
    err = imkdir(inode, filename, mode);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

ssize_t freaddir(file_t *dir, off_t off, void *buf, size_t count) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(dir);

    if (dir->fops == NULL || dir->fops->freaddir == NULL)
        goto generic;

    return dir->fops->freaddir(dir, off, buf, count);

generic:
    if (dir->f_dentry == NULL)
        return -ENOENT;

    dlock(dir->f_dentry);
    if ((inode = dir->f_dentry->d_inode)) {
        ilock(inode);
        idupcnt(inode);
    }
    dunlock(dir->f_dentry);

    if (inode == NULL)
        return -ENOENT;
    
    err = ireaddir(inode, off, buf, count);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     flinkat(file_t *dir, const char *oldname, const char *newname) {
    int err = 0;
    inode_t *inode = NULL;

    fassert_locked(dir);

    if (dir->fops == NULL || dir->fops->flinkat == NULL)
        goto generic;

    return dir->fops->flinkat(dir, oldname, newname);

generic:
    if (dir->f_dentry == NULL)
        return -ENOENT;

    dlock(dir->f_dentry);
    if ((inode = dir->f_dentry->d_inode)) {
        ilock(inode);
        idupcnt(inode);
    }
    dunlock(dir->f_dentry);

    if (inode == NULL)
        return -ENOENT;
    
    err = ilink(oldname, inode, newname);
    iputcnt(inode);
    iunlock(inode);
    return err;
}

int     fmknodat(file_t *dir, const char *pathname, mode_t mode, int devid) {
    int err = 0;
    inode_t *inode = NULL;
    __unused char *path = NULL, *filename = NULL;
    if ((err = path_get_lasttoken(pathname, &filename)))
        return err;

    fassert_locked(dir);

    if (dir->fops == NULL || dir->fops->fmknodat == NULL)
        goto generic;

    return dir->fops->fmknodat(dir, filename, mode, devid);

generic:
    if (dir->f_dentry == NULL)
        return -ENOENT;

    dlock(dir->f_dentry);
    if ((inode = dir->f_dentry->d_inode)) {
        ilock(inode);
        idupcnt(inode);
    }
    dunlock(dir->f_dentry);

    if (inode == NULL)
        return -ENOENT;
    
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

    if (__mm_region_read(region)) {
        if (((file->f_oflags & O_ACCMODE) != O_RDONLY)
            && ((file->f_oflags & O_ACCMODE) != O_RDWR))
            return -EACCES;
    }

    if (__mm_region_write(region)) {
        if (((file->f_oflags & O_ACCMODE) != O_WRONLY)
            && ((file->f_oflags & O_ACCMODE) != O_RDWR))
            return -EACCES;
    }

    if (__mm_region_exec(region)) {
        if (!(file->f_oflags & O_EXCL))
            return -EACCES;
    }

    if (file->fops == NULL || file->fops->fmmap == NULL)
        goto generic;
    
    return file->fops->fmmap(file, region);

generic:
    if (file->f_dentry == NULL)
        return -ENOENT;

    dlock(file->f_dentry);
    if ((inode = file->f_dentry->d_inode)) {
        ilock(inode);
        idupcnt(inode);
    }
    dunlock(file->f_dentry);

    if (inode == NULL)
        return -ENOENT;
    
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