#include <fs/file.h>
#include <bits/errno.h>
#include <sys/thread.h>
#include <fs/fs.h>
#include <lib/string.h>
#include <dev/dev.h>
#include <mm/kalloc.h>

int     file_get(int fd, file_t **ref) {
    file_t *file = NULL;
    file_table_t *file_table = NULL;

    if (ref == NULL)
        return -EINVAL;

    /// TODO: Is this lock on current thread
    /// neccessary just for accessing file_table?
    current_lock();
    file_table = current->t_file_table;
    ftlock(file_table);
    current_unlock();

    if ((file_table->ft_file == NULL) || (fd < 0) || (fd >= file_table->ft_fcnt)) {
        ftunlock(file_table);
        return -EBADFD;
    }

    if ((file = file_table->ft_file[fd]) == NULL) {
        ftunlock(file_table);
        return -EBADFD;
    }

    flock(file);
    ftunlock(file_table);

    *ref = file;
    return 0;
}

int     file_free(int fd) {
    file_table_t *file_table = NULL;

    current_lock();
    file_table = current->t_file_table;
    ftlock(file_table);
    current_unlock();

    if ((file_table->ft_file == NULL) || (fd < 0) || (fd >= file_table->ft_fcnt)) {
        ftunlock(file_table);
        return -EBADFD;
    }

    file_table->ft_file[fd] = NULL;
    ftunlock(file_table);
    return 0;
}

int     file_alloc(int *ref, file_t **fref) {
    int fd = 0;
    int err = 0;
    file_t *file = NULL;
    file_t **tmp = NULL;
    file_table_t *file_table = NULL;

    if (ref == NULL || fref == NULL)
        return -EINVAL;

    if ((err = falloc(&file)))
        return err;

    current_lock();
    file_table = current->t_file_table;
    ftlock(file_table);
    current_unlock();

    if (file_table->ft_file == NULL) {
        if ((file_table->ft_file = kcalloc(1, sizeof (file_t *))) == NULL) {
            ftunlock(file_table);
            fdestroy(file);
            return -ENOMEM;
        }

        file_table->ft_fcnt = 1;
    }

    for (fd = 0; fd < file_table->ft_fcnt; ++fd) {
        if (file_table->ft_file[fd] == NULL) {
            file_table->ft_file[fd] = file;
            ftunlock(file_table);
            goto done;
        }
    }


    if ((tmp = krealloc(file_table->ft_file, ((file_table->ft_fcnt + 1) * sizeof (file_t *)))) == NULL) {
        ftunlock(file_table);
        fdestroy(file);
        return -ENOMEM;
    }
    
    tmp[fd] = file;
    file_table->ft_file = tmp;
    file_table->ft_fcnt++;

    ftunlock(file_table);
done:
    *ref = fd;
    *fref = file;
    return 0;
}

int     file_dup(int fd1, int fd2) {
    int err = 0;
    file_table_t *file_table = NULL;
    file_t *file = NULL, **tmp = NULL;

    current_lock();
    file_table = current->t_file_table;
    ftlock(file_table);
    current_unlock();

    if ((file_table->ft_file == NULL)) {
        ftunlock(file_table);
        return -EBADFD;
    }

    if (((file = file_table->ft_file[fd1]) == NULL) || (fd1 < 0) || (fd1 >= file_table->ft_fcnt)) {
        ftunlock(file_table);
        return -EBADFD;
    }

    if (fd2 < 0)
        fd2 = file_table->ft_fcnt;
    
    if (fd1 == fd2) {
        ftunlock(file_table);
        return fd1;
    }

    if (fd2 > 0) {
        if (fd2 >= file_table->ft_fcnt) {
            if ((tmp = krealloc(file_table->ft_file, ((fd2 + 1) * sizeof (file_t *)))) == NULL) {
                ftunlock(file_table);
                return -ENOMEM;
            }
            memset(&tmp[file_table->ft_fcnt], 0, ((fd2 + 1) - file_table->ft_fcnt)*sizeof (file_t *));
            file_table->ft_file = tmp;
            file_table->ft_fcnt = fd2 + 1;
        }

        if (file_table->ft_file[fd2]) {
            flock(file_table->ft_file[fd2]);
            if ((err = fclose(file_table->ft_file[fd2]))) {
                funlock(file_table->ft_file[fd2]);
                ftunlock(file_table);
                return err;
            }
            file_table->ft_file[fd2] = NULL;
        }
    }

    flock(file);
    if ((err = fdup(file))) {
        funlock(file);
        ftunlock(file_table);
        return err;
    }
    funlock(file);

    file_table->ft_file[fd2] = file;
    ftunlock(file_table);

    return fd2;
}

void file_close_all(void) {
    int         file_cnt = 0;
    file_table_t *file_table = NULL;

    current_lock();
    file_table = current->t_file_table;
    ftlock(file_table);
    current_unlock();

    if (file_table->ft_file != NULL)
        file_cnt = file_table->ft_fcnt;
    ftunlock(file_table);

    for (int fd = 0; fd < file_cnt; ++fd)
        close(fd);
}

int file_copy(file_table_t *dst, file_table_t *src) {
    char    *cwdir      = NULL;
    char    *rootdir    = NULL;
    file_t  **file_table= NULL;
    int     err         = -ENOMEM;

    if (dst == NULL || src == NULL)
        return -EINVAL;
    
    ftassert_locked(dst);
    ftassert_locked(src);
    

    if (NULL == (cwdir  = (char *)strdup(src->cred.c_cwd)))
        goto error;

    if (NULL == (rootdir = (char *)strdup(src->cred.c_root)))
        goto error;

    if (src->ft_fcnt != 0) {
        if (!(file_table = (file_t **)kmalloc(src->ft_fcnt * sizeof (file_t *))))
            goto error;
    }

    dst->ft_fcnt        = src->ft_fcnt;

    kfree(dst->cred.c_cwd);
    kfree(dst->cred.c_root);

    dst->cred.c_cwd     = cwdir;
    dst->cred.c_root    = rootdir;
    dst->ft_file        = file_table;
    dst->cred.c_uid     = src->cred.c_uid;
    dst->cred.c_euid    = src->cred.c_euid;
    dst->cred.c_suid    = src->cred.c_suid;
    dst->cred.c_gid     = src->cred.c_gid;
    dst->cred.c_egid    = src->cred.c_egid;
    dst->cred.c_sgid    = src->cred.c_sgid;
    dst->cred.c_umask   = src->cred.c_umask;
    
    for (int fd = 0; fd < src->ft_fcnt; ++fd) {
        if (src->ft_file[fd] == NULL)
            continue;
        
        flock(src->ft_file[fd]);
        src->ft_file[fd]->f_refcnt++;
        dst->ft_file[fd] = src->ft_file[fd];
        funlock(src->ft_file[fd]);
    }

    return 0;
error:
    if (cwdir)
        kfree(cwdir);
    
    if (rootdir)
        kfree(rootdir);
    
    if (file_table)
        kfree(file_table);

    return err;
}

int     open(const char *pathname, int oflags, mode_t mode) {
    int fd = 0;
    int err = 0;
    cred_t *cred = NULL;
    file_t *file = NULL;
    dentry_t *dentry = NULL;

    if ((err = vfs_lookup(pathname, cred, oflags, mode, 0, &dentry)))
        return err;
    

    if ((err = file_alloc(&fd, &file))) {
        dclose(dentry);
        return err;
    }

    file->fops = NULL;
    file->f_dentry = dentry;
    file->f_oflags = oflags;
    
    funlock(file);
    dunlock(dentry);
    return fd;
}

int     dup(int fd) {
    return file_dup(fd, -1);
}

int     dup2(int fd1, int fd2) {
    return file_dup(fd1, fd2);
}

int     sync(int fd) {
    int err = 0;
    file_t *file = NULL;
    if((err = file_get(fd, &file)))
        return err;
    err = fsync(file);
    funlock(file);
    return err;
}

int     close(int fd) {
    int err = 0;
    file_t *file = NULL;
    if((err = file_get(fd, &file)))
        return err;
    if ((err = fclose(file)))
        funlock(file);
    else
        err = file_free(fd);
    return err;    
}

int     unlink(int fd) {
    int err = 0;
    file_t *file = NULL;

    if((err = file_get(fd, &file)))
        return err;
    err = funlink(file);
    funlock(file);
    return err;    
}

int     getattr(int fd, void *attr) {
    int err = 0;
    file_t *file = NULL;

    if((err = file_get(fd, &file)))
        return err;
    err = fgetattr(file, attr);
    funlock(file);
    return err;    
}

int     setattr(int fd, void *attr) {
    int err = 0;
    file_t *file = NULL;

    if((err = file_get(fd, &file)))
        return err;
    err = fsetattr(file, attr);
    funlock(file);
    return err;    
}

int     truncate(int fd, off_t length) {
    int err = 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = ftruncate(file, length);
    funlock(file);
    return err;
}

int     fcntl(int fd, int cmd, void *argp) {
    int err = 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = ffcntl(file, cmd, argp);
    funlock(file);
    return err;
}

int     ioctl(int fd, int req, void *argp) {
    int err = 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    fioctl(file, req, argp);
    funlock(file);
    return err;
}

off_t   lseek(int fd, off_t off, int whence) {
    off_t err= 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = flseek(file, off, whence);
    funlock(file);
    return err;
}

ssize_t read(int fd, void *buf, size_t size) {
    ssize_t err= 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = fread(file, buf, size);
    funlock(file);
    return err;
}

ssize_t write(int fd, void *buf, size_t size) {
    ssize_t err= 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = fwrite(file, buf, size);
    funlock(file);
    return err;
}

int     create(int fd, const char *pathname, mode_t mode) {
    int err= 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = fcreate(file, pathname, mode);
    funlock(file);
    return err;
}

int     mkdirat(int fd, const char *pathname, mode_t mode) {
    int err= 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = fmkdirat(file, pathname, mode);
    funlock(file);
    return err;
}

ssize_t readdir(int fd, off_t off, void *buf, size_t count) {
    ssize_t err= 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = freaddir(file, off, buf, count);
    funlock(file);
    return err;
}

int     linkat(int fd, const char *oldname, const char *newname) {
    int err= 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = flinkat(file, oldname, newname);
    funlock(file);
    return err;
}

int     mknodat(int fd, const char *pathname, mode_t mode, int devid) {
    int err= 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = fmknodat(file, pathname, mode, devid);
    funlock(file);
    return err;
}

int fstat(int fd, struct stat *buf) {
    int     err = 0;
    file_t  *file = NULL;

    if (buf == NULL)
        return -EINVAL;

    if ((err = file_get(fd, &file)))
        return err;
    
    if ((err = file_stat(file, buf))) {
        funlock(file);
        return err;
    }

    funlock(file);
    return 0;
}

int stat(const char *restrict path, struct stat *restrict buf) {
    int         err     = 0;
    dentry_t    *dentry = NULL;
    cred_t      *cred   = NULL;

    if (path == NULL || buf == NULL)
        return -EINVAL;
    
    if ((err = vfs_lookup(path, cred, O_RDONLY, 0, 0, &dentry)))
        return err;
    
    if (dentry->d_inode == NULL) {
        dclose(dentry);
        return -EBADF;
    }

    ilock(dentry->d_inode);
    if ((err = istat(dentry->d_inode, buf))) {
        iunlock(dentry->d_inode);
        dclose(dentry);
        return -EBADF;
    }
    iunlock(dentry->d_inode);

    dclose(dentry);
    return 0;
}

int lstat(const char *restrict path, struct stat *restrict buf) {
    return stat(path, buf);
}

int fstatat(int fd, const char *restrict path, struct stat *restrict buf, int flag) {
    int         err         = 0;
    file_t      *dir_file   = NULL;
    dentry_t    *dentry     = NULL;
    cred_t      *cred       = NULL;
    (void)      flag;

    if ((err = file_get(fd, &dir_file)))
        return err;
    
    if (dir_file->f_dentry == NULL) {
        funlock(dir_file);
        return err;
    }

    dlock(dir_file->f_dentry);
    if ((err = vfs_lookupat(path, dir_file->f_dentry, cred, O_RDONLY, 0, 0, &dentry))) {
        dunlock(dir_file->f_dentry);
        funlock(dir_file);
        return err;
    }
    dunlock(dir_file->f_dentry);
    funlock(dir_file);

    if (dentry->d_inode == NULL) {
        dclose(dentry);
        return -EBADF;
    }

    ilock(dentry->d_inode);
    if ((err = istat(dentry->d_inode, buf))) {
        iunlock(dentry->d_inode);
        dclose(dentry);
        return -EBADF;
    }
    iunlock(dentry->d_inode);

    dclose(dentry);
    return 0;
}
