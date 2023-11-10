#include <fs/file.h>
#include <bits/errno.h>
#include <sys/thread.h>
#include <fs/fs.h>
#include <dev/dev.h>
#include <mm/kalloc.h>

int file_get(int fd, file_t **ref) {
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

    file = ft->ft_file[fd];
    flock(file);
    ftunlock(ft);

    *ref = file;
    return 0;
}

int file_alloc(int *ref, file_t **fref) {
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
    err = fclose(file);
    funlock(file);
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
    int err= 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = fread(file, buf, size);
    funlock(file);
    return err;
}

ssize_t write(int fd, void *buf, size_t size) {
    int err= 0;
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
    int err= 0;
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
