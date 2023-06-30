#pragma once

#include <lib/stdint.h>

typedef int pid_t;
typedef int tid_t;
typedef int uid_t;
typedef int gid_t;
typedef int mode_t;

typedef long ssize_t;


typedef struct cpu cpu_t;
typedef struct thread thread_t;
typedef struct page page_t;
typedef struct queue queue_t;
typedef struct spinlock spinlock_t;