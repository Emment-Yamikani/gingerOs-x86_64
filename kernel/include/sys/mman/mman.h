#pragma once

#include <lib/stddef.h>
#include <lib/types.h>

extern int getpagesize(void);
extern void *mmap(void *__addr, size_t __len, int __prot,
                    int __flags, int __fd, off_t __offset);

extern int munmap(void *addr, size_t length);
extern int mprotect(void *addr, size_t len, int prot);