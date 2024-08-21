#pragma once

#include <core/defs.h>

// type names used in gingerOS //

typedef char                i8;
typedef short               i16;
typedef int                 i32;
typedef long                i64;
typedef long                isize;

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long       u64;
typedef unsigned long       usize;
typedef unsigned long       uintptr_t;

typedef int                 tid_t;
typedef int                 pid_t;

typedef int                 uid_t;
typedef int                 gid_t;
typedef int                 mode_t;

// represents the CPU local storage.
typedef struct cpu_t        cpu_t;

// a thread(executable entity) struct.
typedef struct thread_t     thread_t;

typedef struct spinlock_t   spinlock_t;