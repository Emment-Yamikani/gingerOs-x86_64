#pragma once

#include <lib/stdint.h>

#include <sys/system.h>

#define PG_X            BS(0)   // page is executable.
#define PG_R            BS(1)   // page is readable.
#define PG_W            BS(2)   // page is writable.
#define PG_U            BS(3)   // page is user.
#define PG_VALID        BS(4)   // page is valid in physical.
#define PG_SHARED       BS(5)   // shared page.

#define PG_D            BS(6)   // page is dirty.
#define PG_WRITEBACK    BS(7)   // page needs writeback
#define PG_SWAPPABLE    BS(8)   // page swapping is allowed.
#define PG_SWAPPED      BS(9)   // page is swapped out.
#define PG_L            BS(10)  // page is locked in memory.
#define PG_C            BS(11)  // page is cached.

#define PG_RX           (PG_R | PG_X)
#define PG_RW           (PG_R | PG_W)
#define PG_RWX          (PG_RW| PG_X)