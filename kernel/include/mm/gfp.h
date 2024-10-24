#pragma once

#include <lib/types.h>

typedef u32 gfp_t;

#define __GFP_ANY           0x00000
#define __GFP_DMA           0x00001
#define __GFP_NORMAL        0x00002
#define __GFP_HOLE          0x00003
#define __GFP_HIGHMEM       0x00004

#define __GFP_ATOMIC        0x00008
#define __GFP_RECLAIMABLE   0x00010
#define __GFP_NOWARN        0x00020
#define __GFP_NOFAIL        0x00040
#define __GFP_NOFALLBACK    0x00080
#define __GFP_WAIT          0x00100
#define __GFP_FS            0x00200
#define __GFP_IO            0x00400
#define __GFP_RETRY         0x00800
#define __GFP_ZERO          0x01000

#define GFP_WAIT            (__GFP_WAIT)
#define GFP_FS              (__GFP_FS)
#define GFP_IO              (__GFP_IO)
#define GFP_RETRY           (__GFP_RETRY)
#define GFP_ZERO            (__GFP_ZERO)

/*This is an allocation from ZONE_DMA. Device drivers that need
DMA-able memory use this flag, usually in combination with one of
the preceding flags.*/
#define GFP_DMA             (__GFP_DMA)
#define GFP_NORMAL          (__GFP_NORMAL)
#define GFP_HOLE            (__GFP_HOLE)
#define GFP_HIGH            (__GFP_HIGHMEM)

#define GFP_WHENCE(gfp)     ((gfp) & 0x0F)


/**The allocation is high priority and must not sleep.
 * This is the flag to use in interrupt handlers,
 * while holding a spinlock, and in other situations
 * where you cannot sleep.*/
#define GFP_ATOMIC          __GFP_HIGHMEM

/*Like GFP_ATOMIC, except that the call will not fallback on emer-
gency memory pools. This increases the liklihood of the memory
allocation failing.*/
#define GFP_NOWAIT          0

/*This allocation can block, but must not initiate disk I/O. This is the
flag to use in block I/O code when you cannot cause more disk
I/O, which might lead to some unpleasant recursion.*/
#define GFP_NOIO            __GFP_WAIT

/*This allocation can block and can initiate disk I/O, if it must, but it
will not initiate a filesystem operation. This is the flag to use in
filesystem code when you cannot start another filesystem operation.*/
#define GFP_NOFS            (__GFP_WAIT | __GFP_IO)

/*This is a normal allocation and might block. This is the flag to use
in process context code when it is safe to sleep. The kernel will do
whatever it has to do to obtain the memory requested by the
caller. This flag should be your default choice.*/
#define GFP_KERNEL          (__GFP_NORMAL | __GFP_WAIT | __GFP_IO | __GFP_FS)

/*This is a normal allocation and might block. This flag is used to
allocate memory for user-space processes.*/
#define GFP_USER            (__GFP_NORMAL | __GFP_WAIT | __GFP_IO | __GFP_FS)

/*This is an allocation from ZONE_HIGHMEM and might block. This
flag is used to allocate memory for user-space processes.*/
#define GFP_HIGHUSER        (__GFP_WAIT | __GFP_IO | __GFP_FS | __GFP_HIGHMEMMEM)
