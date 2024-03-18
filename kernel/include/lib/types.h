#pragma once

#include <lib/stdint.h>

typedef     unsigned char       u8;
typedef     unsigned short      u16;
typedef     unsigned int        u32;
typedef     unsigned long       u64;

typedef     char                i8;
typedef     short               i16;
typedef     int                 i32;
typedef     long                i64;

typedef     unsigned char       flags8_t;    // 8-bit flags.
typedef     unsigned short      flags16_t;   // 16-bit flags.
typedef     unsigned int        flags32_t;   // 32-bit flags.
typedef     unsigned long       flags64_t;   // 64-bit flags.

typedef     int                 pid_t;
typedef     int                 tid_t;

typedef     unsigned long       off_t;
typedef     int                 uid_t;
typedef     int                 gid_t;
typedef     int                 ino_t;
typedef     int                 mode_t;
typedef     long                time_t;
typedef     long                timer_t;
typedef     long                clock_t;
typedef     long                clockid_t;
typedef     int                 susseconds_t;

struct devid {
    uint8_t     major;
    uint8_t     minor;
    uint8_t     type;
};

typedef     uint16_t            devid_t;
typedef     signed long         ssize_t;

typedef     struct inode        *INODE;
typedef     struct inode        inode_t;

struct devid;
typedef     struct dev          dev_t;

typedef     struct cpu          cpu_t;
typedef     struct ipi_t        ipi_t;

typedef     struct              proc proc_t;
typedef     struct              thread_t thread_t;

typedef     struct              vmr vmr_t;
typedef     struct              page page_t;
typedef     struct vm_fault_t   vm_fault_t;

typedef     struct queue        queue_t;

typedef     struct __spinlock_t spinlock_t;
typedef     enum   tstate_t     tstate_t;
typedef     void   *(*thread_entry_t)(void *);
typedef     struct arch_thread_t arch_thread_t;