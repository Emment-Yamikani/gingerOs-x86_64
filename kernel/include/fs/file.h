#pragma once
#include <lib/types.h>
#include <fs/dentry.h>
#include <sync/spinlock.h>
#include <sync/assert.h>
#include <fs/stat.h>
#include <fs/fcntl.h> 

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
    int     (*fcreate)(file_t *dir, const char *pathname, mode_t mode);
    int     (*fmkdirat)(file_t *dir, const char *pathname, mode_t mode);
    ssize_t (*freaddir)(file_t *dir, off_t off, void *buf, size_t count);
    int     (*flinkat)(file_t *dir, const char *oldname, const char *newname);
    int     (*fmknodat)(file_t *dir, const char *pathname, mode_t mode, int devid);
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

int falloc(file_t **pfp);
void fdestroy(file_t *file);

int     feof(file_t *file);
int     fsync(file_t *file);
int     fclose(file_t *file);
int     funlink(file_t *file);
int     fgetattr(file_t *file, void *attr);
int     fsetattr(file_t *file, void *attr);
int     ftruncate(file_t *file, off_t length);
int     ffcntl(file_t *file, int cmd, void *argp);
int     fioctl(file_t *file, int req, void *argp);
off_t   flseek(file_t *file, off_t off, int whence);
ssize_t fread(file_t *file, void *buf, size_t size);
ssize_t fwrite(file_t *file, void *buf, size_t size);
int     fcreate(file_t *dir, const char *pathname, mode_t mode);
int     fmkdirat(file_t *dir, const char *pathname, mode_t mode);
ssize_t freaddir(file_t *dir, off_t off, void *buf, size_t count);
int     flinkat(file_t *dir, const char *oldname, const char *newname);
int     fmknodat(file_t *dir, const char *pathname, mode_t mode, int devid);


int     fsymlink(file_t *file, file_t *atdir, const char *symname);
int     fbind(file_t *dir, struct dentry *dentry, inode_t *file);


typedef struct file_table_t {
    file_t      **ft_file;  // file descriptor table.
    int         ft_fcnt;   // No. of file descriptors in the table.
    spinlock_t  ft_lock;   // spinlock to guard this table.
} file_table_t;

#define ftassert(ft)        ({ assert(ft, "No file table pointer."); })
#define ftlock(ft)          ({ fassert(ft); spin_lock(&(ft)->ft_lock); })
#define ftunlock(ft)        ({ fassert(ft); spin_unlock(&(ft)->ft_lock); })
#define ftislocked(ft)      ({ fassert(ft); spin_islocked(&(ft)->ft_lock); })
#define ftassert_locked(ft) ({ fassert(ft); spin_assert_locked(&(ft)->ft_lock); })