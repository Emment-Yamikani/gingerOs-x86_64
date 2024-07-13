#include <bits/errno.h>
#include <fs/fs.h>
#include <fs/pipefs.h>
#include <fs/tmpfs.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <sys/thread.h>

static filesystem_t *pipefs = NULL;
__unused static superblock_t *pipefs_sb = NULL;

static iops_t pipefs_iops = {
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
    
    if ((pipe = (pipe_t *)kmalloc(sizeof *pipe)))
        return -ENOMEM;
    
    memset(pipe, 0, sizeof *pipe);
    pipe_lock(pipe);

    if ((err = ringbuf_init(PIPESZ, &pipe->p_ringbuf)))
        goto error;
    
    if ((err = ialloc(FS_FIFO, &pipe->p_iread)))
        goto error;
    
    if ((err = ialloc(FS_FIFO, &pipe->p_iwrite)))
        goto error;
    
    pipe_setflags(pipe, PIPE_RW);

    pipe->p_iread->i_ops    = &pipefs_iops;
    pipe->p_iread->i_mode   = 0444;
    iunlock(pipe->p_iread);

    pipe->p_iwrite->i_mode  = 0222;
    pipe->p_iwrite->i_ops   = &pipefs_iops;
    iunlock(pipe->p_iwrite);

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

int pipefs_iclose(inode_t *ip __unused) {
    return -ENOSYS;
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

ssize_t pipefs_iread(inode_t *ip __unused, off_t off __unused, void *buf __unused, size_t nb __unused) {
    int     err     = 0;
    usize   read    = 0;
    pipe_t  *pipe   = NULL;

    pipe = ip->i_priv;
    pipe_lock(pipe);

    if ((ip->i_mode & S_IREAD) == 0) {
        pipe_unlock(pipe);
        return -EACCES;
    }

    while (read < nb) {
        ringbuf_lock(&pipe->p_ringbuf);

        if (ringbuf_isempty(&pipe->p_ringbuf)) {
            ringbuf_unlock(&pipe->p_ringbuf);
            if (pipe_isreadable(pipe)) {
                pipe_unlock(pipe);   
                return read ? (isize)read : -EPIPE;
            }

            pipe_lock_writersq(pipe);
            sched_wake1(pipe_writersq(pipe));
            pipe_unlock_writersq(pipe);

            current_tgroup_lock();
            current_lock();
            err = sched_sleep_r(pipe_readersq(pipe), T_ISLEEP, &pipe->p_lock);
            current_unlock();
            current_tgroup_unlock();

            if (err != 0) {
                pipe_unlock(pipe);
                return err;
            }

            ringbuf_lock(&pipe->p_ringbuf);
        }

        read += ringbuf_read(&pipe->p_ringbuf, nb - read, buf + read);
        ringbuf_unlock(&pipe->p_ringbuf);
    }

    pipe_unlock(pipe);

    return -ENOSYS;
}

ssize_t pipefs_iwrite(inode_t *ip __unused, off_t off __unused, void *buf __unused, size_t nb __unused) {
    return -ENOSYS;
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