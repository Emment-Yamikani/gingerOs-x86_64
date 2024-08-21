#pragma once

#include <core/types.h>

#define VMABASE         (0xFFFFFF8000000000ul)

#define VMA2LO(x)       ((uintptr_t)(x) - VMABASE)
#define VMA2HI(x)       ((uintptr_t)(x) + VMABASE)

extern void *bzero(void *ptr, usize size);

extern void *fast_memset(void *dest, int c, usize len);