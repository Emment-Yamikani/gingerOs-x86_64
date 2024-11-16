#pragma once

#include <lib/stddef.h>

struct iovec {
   void   *iov_base;  // Base address of a memory region for input or output. 
   size_t  iov_len;   // The size of the memory pointed to by iov_base.
};