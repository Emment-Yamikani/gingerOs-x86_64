#ifndef _STAT_H
#define _STAT_H
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <lib/types.h>


struct  stat  {
  devid_t         st_dev;
  ino_t           st_ino;
  mode_t          st_mode;
  unsigned long   st_nlink;
  uid_t           st_uid;
  gid_t           st_gid;
  devid_t         st_rdev;
  size_t          st_size;
  time_t          st_atime;
  time_t          st_mtime;
  time_t          st_ctime;
};

#define  _IFMT      0170000   // Bit mask for the file type bit fields

#define  _IFDIR     0040000  // Directory
#define  _IFCHR     0020000  // Character device
#define  _IFBLK     0060000  // Block device
#define  _IFREG     0100000  // Regular file
#define  _IFIFO     0010000  // FIFO or pipe
#define  _IFLNK     0120000  // Symbolic link
#define  _IFSOCK    0140000  // Socket

#define     S_BLKSIZE  1024 /* size of a block */

#define S_ISUID     0004000 /* set user id on execution */
#define S_ISGID     0002000 /* set group id on execution */
#define S_ISVTX     0001000 /* save swapped text even after use */
#define S_IREAD     0000400 /* read permission, owner */
#define S_IWRITE    0000200 /* write permission, owner */
#define S_IEXEC     0000100 /* execute/search permission, owner */
#define S_ENFMT     0002000 /* enforcement-mode locking */

#define S_IFMT      _IFMT
#define S_IFDIR     _IFDIR
#define S_IFCHR     _IFCHR
#define S_IFBLK     _IFBLK
#define S_IFREG     _IFREG
#define S_IFLNK     _IFLNK
#define S_IFSOCK    _IFSOCK
#define S_IFIFO     _IFIFO

#define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)   // Test for a directory
#define S_ISCHR(mode)  (((mode) & S_IFMT) == S_IFCHR)   // Test for a character special file
#define S_ISBLK(mode)  (((mode) & S_IFMT) == S_IFBLK)   // Test for a block special file
#define S_ISREG(mode)  (((mode) & S_IFMT) == S_IFREG)   // Test for a regular file
#define S_ISFIFO(mode) (((mode) & S_IFMT) == S_IFIFO)   // Test for a FIFO or pipe
#define S_ISLNK(mode)  (((mode) & S_IFMT) == S_IFLNK)   // Test for a symbolic link
#define S_ISSOCK(mode) (((mode) & S_IFMT) == S_IFSOCK)  // Test for a socket

#define S_IRWXU     (S_IRUSR | S_IWUSR | S_IXUSR)
#define     S_IRUSR 0000400 /* read permission, owner */
#define     S_IWUSR 0000200 /* write permission, owner */
#define     S_IXUSR 0000100/* execute/search permission, owner */
#define S_IRWXG     (S_IRGRP | S_IWGRP | S_IXGRP)
#define     S_IRGRP 0000040 /* read permission, group */
#define     S_IWGRP 0000020 /* write permission, grougroup */
#define     S_IXGRP 0000010/* execute/search permission, group */
#define S_IRWXO     (S_IROTH | S_IWOTH | S_IXOTH)
#define     S_IROTH 0000004 /* read permission, other */
#define     S_IWOTH 0000002 /* write permission, other */
#define     S_IXOTH 0000001/* execute/search permission, other */

int fstat(int fd, struct stat *buf);
int stat(const char *restrict path, struct stat *restrict buf);
int lstat(const char *restrict path, struct stat *restrict buf);
int fstatat(int fd, const char *restrict path, struct stat *restrict buf, int flag);

#endif /* _STAT_H */