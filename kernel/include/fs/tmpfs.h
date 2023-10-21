#pragma once

int tmpfs_init(void);

int     tmpfs_isync(inode_t *ip);
int     tmpfs_iclose(inode_t *ip);
int     tmpfs_iunlink(inode_t *ip);
int     tmpfs_ibind(inode_t *dir, struct dentry *dentry, inode_t *ip);
int     tmpfs_ilink(struct dentry *oldname, inode_t *dir, struct dentry *newname);
ssize_t tmpfs_iread(inode_t *ip, off_t off, void *buf, size_t nb);
ssize_t tmpfs_iwrite(inode_t *ip, off_t off, void *buf, size_t nb);
int     tmpfs_imknod(inode_t *dir, struct dentry *dentry, mode_t mode, int devid);
int     tmpfs_ifcntl(inode_t *ip, int cmd, void *argp);
int     tmpfs_iioctl(inode_t *ip, int req, void *argp);
int     tmpfs_imkdir(inode_t *dir, const char *fname, mode_t mode);
int     tmpfs_ilookup(inode_t *dir, const char *fname, inode_t **pipp);
int     tmpfs_icreate(inode_t *dir, const char *fname, mode_t mode);
int     tmpfs_irename(inode_t *dir, struct dentry *old, inode_t *newdir, struct dentry *new);
ssize_t tmpfs_ireaddir(inode_t *dir, off_t off, void *buf, size_t count);
int     tmpfs_isymlink(inode_t *ip, inode_t *atdir, const char *symname);
int     tmpfs_igetattr(inode_t *ip, void *attr);
int     tmpfs_isetattr(inode_t *ip, void *attr);
int     tmpfs_itruncate(inode_t *ip);