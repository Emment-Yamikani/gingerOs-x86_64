#pragma once

typedef     int                 pid_t;
typedef     int                 tid_t;

typedef     long                ssize_t;
typedef     unsigned            long off_t;
typedef     unsigned short      devid_t;
typedef     unsigned short      dev_t;

typedef     int                 uid_t;
typedef     int                 gid_t;
typedef     int                 ino_t;
typedef     int                 mode_t;
typedef     long                time_t;
typedef     int                 suseconds_t;
typedef     long                timer_t;
typedef     long                clock_t;
typedef     long                clockid_t;

typedef unsigned long   useconds_t;
typedef int             pid_t;

#define FD_SETSIZE 64 /* compatibility with newlib */
typedef unsigned int fd_mask;
typedef struct _fd_set
{
    fd_mask fds_bits[1]; /* should be 64 bits */
} fd_set;

typedef     void *(*thread_entry_t)(void *);

typedef     char                i8;
typedef     short               i16;
typedef     int                 i32;
typedef     long                i64;
typedef     long                isize;

typedef     unsigned char       u8;
typedef     unsigned short      u16;
typedef     unsigned int        u32;
typedef     unsigned long       u64;
typedef     unsigned long       usize;
typedef     unsigned long       off_t;