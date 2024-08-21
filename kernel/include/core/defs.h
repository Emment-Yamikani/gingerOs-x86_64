#pragma once

#define loop()      for(;;) // an endless loop.

#define nullptr     ((void *)0) // null pointer.

#define NULL        nullptr    // null pointer.

// number of elements in an array of elements.
#define NELEM(a)    (sizeof((a)) / sizeof ((a)[0]))

#define PGSZ        (4096ul)  // unit size of a page used in the kernel.

#define BS(b)       (1ul << (b))

#define __packed__          __attribute__((packed))
#define __maybe_unused__    __attribute__((unused))
#define __fallthrough__     __attribute__((fallthrough))

#define __packed            __packed__
#define __unused            __maybe_unused__
#define __fallthrough       __fallthrough__
