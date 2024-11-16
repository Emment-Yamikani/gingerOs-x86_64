#include <bits/errno.h>
#include <dev/pty.h>
#include <fs/file.h>
#include <lib/string.h>
#include <sys/thread.h>

int ptsname_r(int fd, char buf[], size_t buflen) {
    int     err     = 0;
    inode_t *ip     = NULL;
    file_t  *file   = NULL;
    PTY     pty     = NULL;
    char    name[32];

    if (buf == NULL)
        return -EINVAL;

    if (buflen > sizeof name)
        return -ERANGE;

    if ((err = file_get(fd, &file)))
        return err;
    
    dlock(file->f_dentry);

    if (NULL == (ip = file->f_dentry->d_inode)) {
        dunlock(file->f_dentry);
        funlock(file);
        return -ENOTTY;
    }

    ilock(ip);
    if ((NULL == (pty = (PTY)ip->i_priv)) || !IISPTMX(ip)) {
        iunlock(ip);
        dunlock(file->f_dentry);
        funlock(file);
        return -ENOTTY;
    }

    pty_lock(pty);
    snprintf(name, sizeof name, "/dev/pts/%d", pty->pt_id);
    pty_unlock(pty);

    iunlock(ip);
    dunlock(file->f_dentry);
    funlock(file);

    return copy_to_user(buf, name, buflen);
}
