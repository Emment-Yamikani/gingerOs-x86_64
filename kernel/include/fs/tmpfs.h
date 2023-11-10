#pragma once

int tmpfs_init(void);
int tmpfs_new_inode(itype_t type, inode_t **pip);

int     tmpfs_isync(inode_t *ip);
int     tmpfs_iclose(inode_t *ip);
int     tmpfs_iunlink(inode_t *ip);
int     tmpfs_itruncate(inode_t *ip);
int     tmpfs_igetattr(inode_t *ip, void *attr);
int     tmpfs_isetattr(inode_t *ip, void *attr);
int     tmpfs_ifcntl(inode_t *ip, int cmd, void *argp);
int     tmpfs_iioctl(inode_t *ip, int req, void *argp);
ssize_t tmpfs_iread_data(inode_t *ip, off_t off, void *buf, size_t nb);
ssize_t tmpfs_iwrite_data(inode_t *ip, off_t off, void *buf, size_t nb);
int     tmpfs_imkdir(inode_t *dir, const char *fname, mode_t mode);
int     tmpfs_icreate(inode_t *dir, const char *fname, mode_t mode);
int     tmpfs_ibind(inode_t *dir, struct dentry *dentry, inode_t *ip);
int     tmpfs_ilookup(inode_t *dir, const char *fname, inode_t **pipp);
int     tmpfs_isymlink(inode_t *ip, inode_t *atdir, const char *symname);
int     tmpfs_imknod(inode_t *dir, const char *name, mode_t mode, int devid);
ssize_t tmpfs_ireaddir(inode_t *dir, off_t off, struct dirent *buf, size_t count);
int     tmpfs_ilink(const char *oldname, inode_t *dir, const char *newname);
int     tmpfs_irename(inode_t *dir, const char *old, inode_t *newdir, const char *new);