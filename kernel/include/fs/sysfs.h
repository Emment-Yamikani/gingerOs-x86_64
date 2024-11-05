#pragma once

int     sysfs_init(void);

int     sysfs_iopen(inode_t *ip, inode_t **pip);
int     sysfs_isync(inode_t *ip);
int     sysfs_iclose(inode_t *ip);
int     sysfs_iunlink(inode_t *ip);
int     sysfs_itruncate(inode_t *ip);
int     sysfs_igetattr(inode_t *ip, void *attr);
int     sysfs_isetattr(inode_t *ip, void *attr);
int     sysfs_ifcntl(inode_t *ip, int cmd, void *argp);
int     sysfs_iioctl(inode_t *ip, int req, void *argp);
ssize_t sysfs_iread_data(inode_t *ip, off_t off, void *buf, size_t nb);
ssize_t sysfs_iwrite_data(inode_t *ip, off_t off, void *buf, size_t nb);
int     sysfs_imkdir(inode_t *dir, const char *fname, mode_t mode);
int     sysfs_icreate(inode_t *dir, const char *fname, mode_t mode);
int     sysfs_ibind(inode_t *dir, struct dentry *dentry, inode_t *ip);
int     sysfs_ilookup(inode_t *dir, const char *fname, inode_t **pipp);
int     sysfs_isymlink(inode_t *ip, inode_t *atdir, const char *symname);
int     sysfs_imknod(inode_t *dir, const char *name, mode_t mode, int devid);
ssize_t sysfs_ireaddir(inode_t *dir, off_t off, struct dirent *buf, size_t count);
int     sysfs_ilink(const char *oldname, inode_t *dir, const char *newname);
int     sysfs_irename(inode_t *dir, const char *old, inode_t *newdir, const char *new);