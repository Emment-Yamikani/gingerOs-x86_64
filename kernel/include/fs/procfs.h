#pragma once


int     procfs_init(void);

int     procfs_iopen(inode_t *idev);
int     procfs_isync(inode_t *ip);
int     procfs_iclose(inode_t *ip);
int     procfs_iunlink(inode_t *ip);
int     procfs_itruncate(inode_t *ip);
int     procfs_igetattr(inode_t *ip, void *attr);
int     procfs_isetattr(inode_t *ip, void *attr);
int     procfs_ifcntl(inode_t *ip, int cmd, void *argp);
int     procfs_iioctl(inode_t *ip, int req, void *argp);
ssize_t procfs_iread_data(inode_t *ip, off_t off, void *buf, size_t nb);
ssize_t procfs_iwrite_data(inode_t *ip, off_t off, void *buf, size_t nb);
int     procfs_imkdir(inode_t *dir, const char *fname, mode_t mode);
int     procfs_icreate(inode_t *dir, const char *fname, mode_t mode);
int     procfs_ibind(inode_t *dir, struct dentry *dentry, inode_t *ip);
int     procfs_ilookup(inode_t *dir, const char *fname, inode_t **pipp);
int     procfs_isymlink(inode_t *ip, inode_t *atdir, const char *symname);
int     procfs_imknod(inode_t *dir, const char *name, mode_t mode, int devid);
ssize_t procfs_ireaddir(inode_t *dir, off_t off, struct dirent *buf, size_t count);
int     procfs_ilink(const char *oldname, inode_t *dir, const char *newname);
int     procfs_irename(inode_t *dir, const char *old, inode_t *newdir, const char *new);