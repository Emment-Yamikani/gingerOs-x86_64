#pragma once

#include <lib/stdint.h>

typedef int pid_t;
typedef int tid_t;
typedef int uid_t;
typedef int gid_t;
typedef int ino_t;
typedef int mode_t;
typedef long time_t;
typedef long timer_t;
typedef long clock_t;
typedef long clockid_t;


typedef signed long ssize_t;
typedef uint16_t devid_t;
typedef unsigned long off_t;

typedef struct inode *INODE;
typedef struct inode inode_t;

typedef struct cpu cpu_t;
typedef struct thread thread_t;
typedef struct page page_t;
typedef struct queue queue_t;
typedef struct spinlock spinlock_t;
typedef enum tstate_t tstate_t;