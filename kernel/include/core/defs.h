#pragma once

#ifndef loop
#define loop()      for(;;) // an endless loop.
#endif

#ifndef nullptr
#define nullptr     ((void *)0) // null pointer.
#endif

#ifndef NULL
#define NULL        nullptr    // null pointer.
#endif

#ifndef NELEM
// number of elements in an array of elements.
#define NELEM(a)    (sizeof((a)) / sizeof ((a)[0]))
#endif

#ifndef PGSZ
#define PGSZ        (4096ul)  // unit size of a page used in the kernel.
#endif