#include <fs/file.h>
#include <bits/errno.h>
#include <sys/thread.h>
#include <fs/fs.h>
#include <lib/string.h>
#include <dev/dev.h>
#include <mm/kalloc.h>

int     file_get(int fd, file_t **ref) {
    file_t *file = NULL;
    file_table_t *ft = NULL;

    if (ref == NULL)
        return -EINVAL;

    current_lock();
    tgroup_lock(current_tgroup());
    ft = &current_tgroup()->tg_file_table;
    ftlock(ft);
    tgroup_unlock(current_tgroup());
    current_unlock();

    if ((ft->ft_file == NULL) || (fd < 0) || (fd >= ft->ft_fcnt)) {
        ftunlock(ft);
        return -EBADFD;
    }

    if ((file = ft->ft_file[fd]) == NULL) {
        ftunlock(ft);
        return -EBADFD;
    }

    flock(file);
    ftunlock(ft);

    *ref = file;
    return 0;
}

int     file_free(int fd) {
    file_table_t *ft = NULL;

    current_lock();
    tgroup_lock(current_tgroup());
    ft = &current_tgroup()->tg_file_table;
    ftlock(ft);
    tgroup_unlock(current_tgroup());
    current_unlock();

    if ((ft->ft_file == NULL) || (fd < 0) || (fd >= ft->ft_fcnt)) {
        ftunlock(ft);
        return -EBADFD;
    }

    ft->ft_file[fd] = NULL;
    ftunlock(ft);
    return 0;
}

int     file_alloc(int *ref, file_t **fref) {
    int fd = 0;
    int err = 0;
    file_t *file = NULL;
    file_t **tmp = NULL;
    file_table_t *ft = NULL;

    if (ref == NULL || fref == NULL)
        return -EINVAL;

    if ((err = falloc(&file)))
        return err;

    current_lock();
    tgroup_lock(current_tgroup());
    ft = &current_tgroup()->tg_file_table;
    ftlock(ft);
    tgroup_unlock(current_tgroup());
    current_unlock();

    if (ft->ft_file == NULL) {
        if ((ft->ft_file = kcalloc(1, sizeof (file_t *))) == NULL) {
            ftunlock(ft);
            fdestroy(file);
            return -ENOMEM;
        }

        ft->ft_fcnt = 1;
    }

    for (fd = 0; fd < ft->ft_fcnt; ++fd) {
        if (ft->ft_file[fd] == NULL) {
            ft->ft_file[fd] = file;
            ftunlock(ft);
            goto done;
        }
    }


    if ((tmp = krealloc(ft->ft_file, ((ft->ft_fcnt + 1) * sizeof (file_t *)))) == NULL) {
        ftunlock(ft);
        fdestroy(file);
        return -ENOMEM;
    }
    
    tmp[fd] = file;
    ft->ft_file = tmp;
    ft->ft_fcnt++;

    ftunlock(ft);
done:
    *ref = fd;
    *fref = file;
    return 0;
}

int     file_dup(int fd1, int fd2) {
    int err = 0;
    file_table_t *ft = NULL;
    file_t *file = NULL, **tmp = NULL;

    current_lock();
    tgroup_lock(current_tgroup());
    ft = &current_tgroup()->tg_file_table;
    ftlock(ft);
    tgroup_unlock(current_tgroup());
    current_unlock();

    if ((ft->ft_file == NULL)) {
        ftunlock(ft);
        return -EBADFD;
    }

    if (((file = ft->ft_file[fd1]) == NULL) || (fd1 < 0) || (fd1 >= ft->ft_fcnt)) {
        ftunlock(ft);
        return -EBADFD;
    }

    if (fd2 < 0)
        fd2 = ft->ft_fcnt;
    
    if (fd1 == fd2) {
        ftunlock(ft);
        return fd1;
    }

    if (fd2 > 0) {
        if (fd2 >= ft->ft_fcnt) {
            if ((tmp = krealloc(ft->ft_file, ((fd2 + 1) * sizeof (file_t *)))) == NULL) {
                ftunlock(ft);
                return -ENOMEM;
            }
            memset(&tmp[ft->ft_fcnt], 0, ((fd2 + 1) - ft->ft_fcnt)*sizeof (file_t *));
            ft->ft_file = tmp;
            ft->ft_fcnt = fd2 + 1;
        }

        if (ft->ft_file[fd2]) {
            flock(ft->ft_file[fd2]);
            if ((err = fclose(ft->ft_file[fd2]))) {
                funlock(ft->ft_file[fd2]);
                ftunlock(ft);
                return err;
            }
            ft->ft_file[fd2] = NULL;
        }
    }

    flock(file);
    if ((err = fdup(file))) {
        funlock(file);
        ftunlock(ft);
        return err;
    }
    funlock(file);

    ft->ft_file[fd2] = file;
    ftunlock(ft);

    return fd2;
}

int     open(const char *pathname, int oflags, ...) {
    int fd = 0;
    int err = 0;
    mode_t mode = 0;
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
