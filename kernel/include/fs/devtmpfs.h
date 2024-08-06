#pragma once

#include <fs/stat.h>
#include <fs/inode.h>
#include <fs/dentry.h>
#include <sys/thread.h>
#include <fs/file.h>
#include <dev/dev.h>

int devtmpfs_init(void);

int     dev_isync(inode_t *ip);
int     dev_iclose(inode_t *ip);
int     dev_iunlink(inode_t *ip);
int     dev_itruncate(inode_t *ip);
int     dev_igetattr(inode_t *ip, void *attr);
int     dev_isetattr(inode_t *ip, void *attr);
int     dev_ifcntl(inode_t *ip, int cmd, void *argp);
int     dev_iioctl(inode_t *ip, int req, void *argp);
ssize_t dev_iread(inode_t *ip, off_t off, void *buf, size_t nb);
ssize_t dev_iwrite(inode_t *ip, off_t off, void *buf, size_t nb);
int     dev_imkdir(inode_t *dir, const char *fname, mode_t mode);
int     dev_icreate(inode_t *dir, const char *fname, mode_t mode);
int     dev_ibind(inode_t *dir, struct dentry *dentry, inode_t *ip);
int     dev_ilookup(inode_t *dir, const char *fname, inode_t **pipp);
int     dev_isymlink(inode_t *ip, inode_t *atdir, const char *symname);
int     dev_imknod(inode_t *dir, const char *name, mode_t mode, int devid);
ssize_t dev_ireaddir(inode_t *dir, off_t off, struct dirent *buf, size_t count);
int     dev_ilink(const char *oldname, inode_t *dir, const char *newname);
int     dev_irename(inode_t *dir, const char *old, inode_t *newdir, const char *new);

extern iops_t dev_iops;