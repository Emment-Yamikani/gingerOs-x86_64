#pragma once
#include <lib/types.h>
#include <fs/dentry.h>
#include <sync/spinlock.h>
#include <sync/assert.h>
#include <fs/stat.h>
#include <fs/fcntl.h>
#include <fs/inode.h>
#include <fs/cred.h>
#include <mm/mmap.h>
#include <sys/_utsname.h>

typedef struct file_t file_t;

typedef struct fops_t {
    int     (*feof)(file_t *file);
    int     (*fsync)(file_t *file);
    int     (*fclose)(file_t *file);
    int     (*funlink)(file_t *file);
    int     (*fgetattr)(file_t *file, void *attr);
    int     (*fsetattr)(file_t *file, void *attr);
    int     (*ftruncate)(file_t *file, off_t length);
    int     (*ffcntl)(file_t *file, int cmd, void *argp);
    int     (*fioctl)(file_t *file, int req, void *argp);
    off_t   (*flseek)(file_t *file, off_t off, int whence);
    ssize_t (*fread)(file_t *file, void *buf, size_t size);
    ssize_t (*fwrite)(file_t *file, void *buf, size_t size);
    int     (*fcreate)(file_t *dir, const char *filename, mode_t mode);
    int     (*fmkdirat)(file_t *dir, const char *filename, mode_t mode);
    ssize_t (*freaddir)(file_t *dir, off_t off, void *buf, size_t count);
    int     (*flinkat)(file_t *dir, const char *oldname, const char *newname);
    int     (*fmknodat)(file_t *dir, const char *filename, mode_t mode, int devid);
    int     (*fmmap)(file_t *file, vmr_t *region);
    int     (*fstat)(file_t *file, struct stat *buf);
    int     (*fchown)(file_t *file, uid_t owner, gid_t group);
} fops_t;

typedef struct file_t {
    off_t       f_off;
    int         f_oflags;
    long        f_refcnt;
    dentry_t    *f_dentry;
    fops_t      *fops;
    spinlock_t  f_lock;
} file_t;

#define fassert(file)         ({ assert(file, "No file pointer."); })
#define flock(file)           ({ fassert(file); spin_lock(&(file)->f_lock); })
#define funlock(file)         ({ fassert(file); spin_unlock(&(file)->f_lock); })
#define fislocked(file)       ({ fassert(file); spin_islocked(&(file)->f_lock); })
#define fassert_locked(file)  ({ fassert(file); spin_assert_locked(&(file)->f_lock); })

int     file_alloc(int *ref, file_t **fref);

int     falloc(file_t **pfp);
void    fdestroy(file_t *file);
int     fdup(file_t *file);
int     fput(file_t *file);

int     feof(file_t *file);
int     fsync(file_t *file);
int     fclose(file_t *file);
int     funlink(file_t *file);
int     fgetattr(file_t *file, void *attr);
int     fsetattr(file_t *file, void *attr);
int     file_stat(file_t *file, struct stat *buf);
int     file_chown(file_t *file, uid_t owner, gid_t group);

int     ftruncate(file_t *file, off_t length);
int     ffcntl(file_t *file, int cmd, void *argp);
int     fioctl(file_t *file, int req, void *argp);
off_t   flseek(file_t *file, off_t off, int whence);
ssize_t fread(file_t *file, void *buf, size_t size);
ssize_t fwrite(file_t *file, void *buf, size_t size);
int     fcreate(file_t *dir, const char *filename, mode_t mode);
int     fmkdirat(file_t *dir, const char *filename, mode_t mode);
ssize_t freaddir(file_t *dir, off_t off, void *buf, size_t count);
int     flinkat(file_t *dir, const char *oldname, const char *newname);
int     fmknodat(file_t *dir, const char *filename, mode_t mode, int devid);
int     fmmap(file_t *file, vmr_t *region);

int     fsymlink(file_t *file, file_t *atdir, const char *symname);
int     fbind(file_t *dir, struct dentry *dentry, inode_t *file);

#define NFILE   1024    // file count.

typedef struct file_ctx_t {
    dentry_t    *fc_cwd;      // current working directory of this file contect.
    int         fc_fmax;      // file context's allowed maximum for open files.
    dentry_t    *fc_root;     // root directory of this file context.
    int         fc_nfile;     // file context's open file count.
    file_t      **fc_files;   // file context's open file table(array).
    spinlock_t  fc_lock;      // spinlock to guard this file context.
} file_ctx_t;

#define fctx_assert(fctx)        ({ assert(fctx, "No file table pointer."); })
#define fctx_lock(fctx)          ({ fassert(fctx); spin_lock(&(fctx)->fc_lock); })
#define fctx_unlock(fctx)        ({ fassert(fctx); spin_unlock(&(fctx)->fc_lock); })
#define fctx_islocked(fctx)      ({ fassert(fctx); spin_islocked(&(fctx)->fc_lock); })
#define fctx_assert_locked(fctx) ({ fassert(fctx); spin_assert_locked(&(fctx)->fc_lock); })

int     fctx_alloc(file_ctx_t**ret);
void    fctx_free(file_ctx_t *fctx);

void    file_close_all(void);
int     file_copy(file_ctx_t *dst, file_ctx_t *src);

int     file_get(int fd, file_t **ref);

int     dup(int fd);
int     sync(int fd);
int     close(int fd);
int     unlink(int fd);
int     dup2(int fd1, int fd2);
int     getattr(int fd, void *attr);
int     setattr(int fd, void *attr);
int     truncate(int fd, off_t length);
int     fcntl(int fd, int cmd, void *argp);
int     ioctl(int fd, int req, void *argp);

/* SEEK_SET */
#define SEEK_SET 0
/* SEEK_CUR */
#define SEEK_CUR 1
/* SEEK_END */
#define SEEK_END 2

int     isatty(int fd);
mode_t  umask(mode_t cmask);
int     pipe(int fds[2]);
off_t   lseek(int fd, off_t off, int whence);
ssize_t read(int fd, void *buf, size_t size);
ssize_t write(int fd, void *buf, size_t size);
int     create(const char *filename, mode_t mode);
int     mkdirat(int fd, const char *filename, mode_t mode);
int     mkdir(const char *filename, mode_t mode);
ssize_t readdir(int fd, off_t off, void *buf, size_t count);
int     open(const char *pathname, int oflags, mode_t mode);
int     openat(int fd, const char *pathn, int oflags, mode_t);
int     linkat(int fd, const char *oldname, const char *newname);
int     openat(int fd, const char *pathname, int oflags, mode_t mode);
int     mknodat(int fd, const char *filename, mode_t mode, int devid);
int     mknod(const char *filename, mode_t mode, int devid);