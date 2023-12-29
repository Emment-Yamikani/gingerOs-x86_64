#pragma once

#include <lib/stdint.h>

#include <sys/system.h>

#define PG_EXEC         (BS(0)) // executable?.
#define PG_WRITE        (BS(1)) // writable?.
#define PG_READ         (BS(2)) // readable?.
#define PG_USER         (BS(3)) // user page?.
#define PG_VALID        (BS(4)) // page is valid in physical.
#define PG_SHARED       (BS(5)) // shared page.
#define PG_DIRTY        (BS(6)) // page is dirty
#define PG_WRITEBACK    (BS(7)) // page needs writeback
#define PG_SWAPPABLE    (BS(8)) // page swapping is allowed.
#define PG_SWAPPED      (BS(9)) // page is swapped out.


#define PG_RX           (PG_READ | PG_EXEC)
#define PG_RW           (PG_READ | PG_WRITE)
#define PG_RWX          (PG_RW   | PG_EXEC)