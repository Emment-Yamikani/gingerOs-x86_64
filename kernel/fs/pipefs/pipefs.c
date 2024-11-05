#include <bits/errno.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <fs/pipefs.h>
#include <fs/tmpfs.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <sys/thread.h>
#include <sys/sysproc.h>
#include <sys/_signal.h>

static filesystem_t *pipefs = NULL;
__unused static superblock_t *pipefs_sb = NULL;

static iops_t pipefs_iops = {
    .iopen      = pipefs_iopen,
    .isync      = pipefs_isync,
    .iclose     = pipefs_iclose,
    .iunlink    = pipefs_iunlink,
    .itruncate  = pipefs_itruncate,
    .igetattr   = pipefs_igetattr,
    .isetattr   = pipefs_isetattr,
    .ifcntl     = pipefs_ifcntl,
    .iioctl     = pipefs_iioctl,
    .iread      = pipefs_iread,
    .iwrite     = pipefs_iwrite,
    .imkdir     = pipefs_imkdir,
    .icreate    = pipefs_icreate,
    .ibind      = pipefs_ibind,
    .ilookup    = pipefs_ilookup,
    .isymlink   = pipefs_isymlink,
    .imknod     = pipefs_imknod,
    .ireaddir   = pipefs_ireaddir,
    .ilink      = pipefs_ilink,
    .irename    = pipefs_irename,
};

static int pipefs_fill_sb(filesystem_t *fs __unused, const char *target,
                         struct devid *devid __unused, superblock_t *sb) {
    int         err    = 0;
    inode_t     *iroot = NULL;
    dentry_t    *droot = NULL;

    if ((err = tmpfs_new_inode(FS_DIR, &iroot)))
        return err;

    if ((err = dalloc(target, &droot))) {
        irelease(iroot);
        return err;
    }

    if ((err = iadd_alias(iroot, droot))) {
        dclose(droot);
        irelease(iroot);
        return err;
    }

    sb->sb_blocksize = -1;
    strncpy(sb->sb_magic0, "pipefs", 10);
    sb->sb_size     = -1;
    sb->sb_root     = droot;
    sb->sb_uio      = (cred_t) {
        .c_gid      = 0,
        .c_uid      = 0,
        .c_umask    = 0776,
        .c_lock     = SPINLOCK_INIT(),
    };

    iroot->i_sb     = sb;
    iroot->i_type   = FS_DIR;
    iroot->i_ops    = sb->sb_iops;

    dunlock(droot);
    iunlock(iroot);
    return 0;
}

static int pipefs_getsb(filesystem_t *fs, const char *src __unused, const char *target,
                       unsigned long flags, void *data, superblock_t **psbp) {
    return getsb_nodev(fs, target, flags, data, psbp, pipefs_fill_sb);
}

int pipefs_init(void) {
    int err = 0;

    if ((err = fs_create("pipefs", &pipefs_iops, &pipefs)))
        return err;

    pipefs->get_sb = pipefs_getsb;
    pipefs->mount = NULL;

    if ((err = vfs_register_fs(pipefs))) {
        fsunlock(pipefs);
        goto error;
    }

    fsunlock(pipefs);
    return 0;
error:
    if (pipefs)
        fs_free(pipefs);
    return err;
}

int pipe_mkpipe(pipe_t **pref) {
    int     err     = 0;
    pipe_t  *pipe   = NULL;

    if (pref == NULL)
        return -EINVAL;
    
    if (NULL == (pipe = (pipe_t *)kmalloc(sizeof *pipe)))
        return -ENOMEM;
    
    memset(pipe, 0, sizeof *pipe);
    pipe_lock(pipe);

    if ((err = ringbuf_init(PIPESZ, pipe_getbuff(pipe))))
        goto error;
    
    if ((err = ialloc(FS_PIPE, I_NORWQUEUES, &pipe->p_iread)))
        goto error;
    
    if ((err = ialloc(FS_PIPE, I_NORWQUEUES, &pipe->p_iwrite)))
        goto error;

    pipe->p_iread->i_priv   = pipe;
    pipe->p_iread->i_mode   = 0444;
    pipe->p_iread->i_ops    = &pipefs_iops;

    pipe->p_iwrite->i_priv  = pipe;
    pipe->p_iwrite->i_mode  = 0222;
    pipe->p_iwrite->i_ops   = &pipefs_iops;

    pipe_setflags(pipe, PIPE_RW);

    *pref = pipe;
    return 0;
error:
    if (pipe) {
        if (pipe->p_iread)
            irelease(pipe->p_iread);
        if (pipe->p_iwrite)
            irelease(pipe->p_iwrite);
        
        if (pipe->p_ringbuf.buf)
            kfree(pipe->p_ringbuf.buf);
        kfree(pipe);
    }
    printk("Failed to create a new pipe, err= %d\n", err);
    return err;
}

int pipefs_isync(inode_t *ip __unused) {
    return -ENOSYS;
}

int pipefs_iopen(inode_t *ip __unused, inode_t **pip __unused) {
    return 0;
}

int pipefs_iclose(inode_t *ip) {
    pipe_t *pipe = NULL;

    pipe = (pipe_t *)ip->i_priv;


    pipe_lock(pipe);

    if (pipe->p_iwrite == ip) { // write end.
        pipe_maskflags(pipe, PIPE_W);
        pipe_wake_all_readers(pipe);
        pipe_wake_all_writers(pipe);
    } else if (pipe->p_iread == ip) { // read end.
        pipe_maskflags(pipe, PIPE_R);
        pipe_wake_all_writers(pipe);
        pipe_wake_all_readers(pipe);
    } else
        panic("Invalid end of pipe.\n");

    pipe_unlock(pipe);

    return 0;
}

int pipefs_iunlink(inode_t *ip __unused) {
    return -ENOSYS;
}

int pipefs_itruncate(inode_t *ip __unused) {
    return -ENOSYS;
}

int pipefs_igetattr(inode_t *ip __unused, void *attr __unused) {
    return -ENOSYS;
}

int pipefs_isetattr(inode_t *ip __unused, void *attr __unused) {
    return -ENOSYS;
}

int pipefs_ifcntl(inode_t *ip __unused, int cmd __unused, __unused void *argp __unused) {
    return -ENOSYS;
}

int pipefs_iioctl(inode_t *ip __unused, int req __unused, void *argp __unused) {
    return -ENOTTY;
}

ssize_t pipefs_iread(inode_t *ip, off_t off __unused, void *buf, size_t nb) {
    int     err     = 0;
    isize   read    = 0;
    pipe_t  *pipe   = NULL;

    pipe = (pipe_t *)ip->i_priv;
    pipe_lock(pipe);

    while (read < (isize)nb) {
        if (!pipe_isreadable(pipe)) {
            pipe_wake_writer(pipe);
            pipe_unlock(pipe);
            kill(getpid(), SIGPIPE);
            return read ? read : -EPIPE;
        }

        pipe_lockbuf(pipe);
        if (ringbuf_isempty(pipe_getbuff(pipe))) {
            pipe_unlockbuf(pipe);
            pipe_wake_writer(pipe);
            if ((err = pipe_reader_wait(pipe))) {
                pipe_unlock(pipe);
                return err;
            }
            pipe_lockbuf(pipe);
        }

        read += ringbuf_read(pipe_getbuff(pipe), buf + read, nb - read);
        pipe_unlockbuf(pipe);
        pipe_wake_writer(pipe);
    }

    pipe_unlock(pipe);
    return read;
}

ssize_t pipefs_iwrite(inode_t *ip, off_t off __unused, void *buf, size_t nb) {
    int     err     = 0;
    isize   written = 0;
    pipe_t  *pipe   = NULL;

    pipe = (pipe_t *)ip->i_priv;
    pipe_lock(pipe);

    while (written < (isize)nb) {
        if (!pipe_iswritable(pipe)) {
            pipe_wake_reader(pipe);
            pipe_unlock(pipe);
            kill(getpid(), SIGPIPE);
            return written ? written : -EPIPE;
        }

        pipe_lockbuf(pipe);
        if (ringbuf_isfull(pipe_getbuff(pipe))) {
            pipe_unlockbuf(pipe);
            pipe_wake_reader(pipe);
            if ((err = pipe_writer_wait(pipe))) {
                pipe_unlock(pipe);
                return err;
            }
            pipe_lockbuf(pipe);
        }
        
        written += ringbuf_write(pipe_getbuff(pipe), buf + written, nb - written);
        pipe_unlockbuf(pipe);
        pipe_wake_reader(pipe);
    }

    pipe_unlock(pipe);
    return written;
}

int pipefs_imkdir(inode_t *dir __unused, const char *fname __unused, mode_t mode __unused) {
    return -ENOSYS;
}

int pipefs_icreate(inode_t *dir __unused, const char *fname __unused, mode_t mode __unused) {
    return -ENOSYS;
}

int pipefs_ibind(inode_t *dir __unused, struct dentry *dentry __unused, inode_t *ip __unused) {
    return -ENOSYS;
}

int pipefs_ilookup(inode_t *dir __unused, const char *fname __unused, inode_t **pipp __unused) {
    return -ENOSYS;
}

int pipefs_isymlink(inode_t *ip __unused, inode_t *atdir __unused, const char *symname __unused) {
    return -ENOSYS;
}

int pipefs_imknod(inode_t *dir __unused, const char *name __unused, mode_t mode __unused, int devid __unused) {
    return -ENOSYS;
}

ssize_t pipefs_ireaddir(inode_t *dir __unused, off_t off __unused, struct dirent *buf __unused, size_t count __unused) {
    return -ENOSYS;
}

int pipefs_ilink(const char *oldname __unused, inode_t *dir __unused, const char *newname __unused) {
    return -ENOSYS;
}

int pipefs_irename(inode_t *dir __unused, const char *old __unused, inode_t *newdir __unused, const char *new __unused) {
    return -ENOSYS;
}