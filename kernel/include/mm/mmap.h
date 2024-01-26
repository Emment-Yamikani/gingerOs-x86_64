#pragma once

#include <arch/paging.h>
#include <bits/errno.h>
#include <lib/stdint.h>
#include <lib/stddef.h>
#include <lib/string.h>
#include <lib/types.h>
#include <mm/page.h>
#include <sync/spinlock.h>
#include <sys/system.h>

#ifndef foreach
#define foreach(elem, list) \
    for (typeof(*list) *tmp = list, elem = *tmp; elem; elem = *++tmp)
#endif //foreach

#ifndef forlinked
#define forlinked(elem, list, iter) \
    for (typeof(list) elem = list; elem; elem = iter)
#endif //forlinked

#ifndef __unused

#ifndef __attribute_maybe_unused__
#define __attribute_maybe_unused__ __attribute__ ((unused))
#endif //__attribute_maybe_unused__

#define __unused __attribute_maybe_unused__

#endif

struct vmr;
struct mmap;

typedef struct vm_fault_t {
    uintptr_t   addr;
    pte_t       *COW;
    pte_t       *page;
    uint8_t     user : 1;
    uintptr_t    err_code;
}vm_fault_t;

typedef struct vmr_ops {
    int (*io)();
    int (*fault)(struct vmr *vmr, vm_fault_t *fault);
}vmr_ops_t;

typedef struct mm_region {
    /// TODO: use this struct in favor of vmr_t
} mm_region_t;

typedef struct vmr {
    long             refs;      // No. of references to this struct.
    int              flags;     // Flags associated with this memory mapping.
    int              vflags;    // Flags used to map this region to physical memory by the paging logic.
    void             *priv;     // Private data(for module and driver-specif use).
    inode_t          *file;     // File used as backing store for this memory region.
    size_t           filesz;    // Size of this region on the file.
    size_t           memsz;     // Size of memory region as taken from program headers.
    off_t            file_pos;  // Position of this region in the file.
    struct mmap      *mmap;     // Memory map associated with this memory region.
    struct vmr_ops   *vmops;    // virtual memory operations that apply to this region.
    uintptr_t        paddr;     // physical address of this memory region(mapping).
    uintptr_t        start;     // Starting address(virtual) of this memory region.
    uintptr_t        end;       // Ending address of this memory region.
    struct vmr       *prev;     // Previous memory object in the list of memory regions.
    struct vmr       *next;     // Next memory object in the list of memory regions.
} vmr_t;

#define MMAP_USER                   1

/*Page size*/
#ifndef PAGESZ
#define PAGESZ                      (0x1000)
#endif

/*Page mask*/
#ifndef PAGEMASK
#define PAGEMASK                    (0x0FFF)
#endif

/*Address Space limit (last valid address)*/
#define __mmap_limit                (USTACK - 1)

/*Returns the value smaller between 'a' and 'b'*/
#if defined MIN
    #define __min(a, b)             MIN(a, b)
#else
    #define __min(a, b)            (((a) < (b) ? (a) : (b)))
#endif

/*Is the address Page aligned?*/
#define __isaligned(p)              (((p) & PAGEMASK) == 0)

/*Size of a memory mapping*/
#define __vmr_size(r)               (((r)->end - (r)->start) + 1)

/*size of region in file on-disk*/
#define __vmr_filesz(r)             (((r)->filesz))

#define __vmr_memsz(r)              (((r)->memsz))

/**Upper edge of a memory mapping.
 **NB*: 'edge' is not part of the mapping*/
#define __vmr_upper_bound(r)        ((r)->end + 1)

/**Lower edge of a memory mapping.
 **NB*: 'edge' is not part of the mapping*/
#define __vmr_lower_bound(r)        ((r)->start - 1)

/*Next mapping in the list*/
#define __vmr_next(r)               ((r)->next)

/*Previous mapping in the list*/
#define __vmr_prev(r)               ((r)->prev)

#define __vmr_start(r)              ((r)->start)
#define __vmr_end(r)                ((r)->end)

#define __vmr_filepos(r)            ((r)->file_pos)           

typedef struct mmap {
    int         flags;      // memory map flags.
    long        refs;       // reference count.
    void       *priv;       // private data.
    vmr_t      *arg;        // region designated for argument vector.
    vmr_t      *env;        // region designated for environment varaibles.
    vmr_t      *heap;       // region dedicated to the heap.
    uintptr_t   brk;        // brk position.
    uintptr_t   pgdir;      // page directory.
    uintptr_t   limit;      // Highest allowed address in this address space.
    size_t      guard_len;  // Size of the guard space.
    size_t      used_space; // Avalable space, may be non-contigous.
    vmr_t      *vmr_head;   // head of list of virtual memory mapping.
    vmr_t      *vmr_tail;   // tail of list of virtual memory mapping.
    spinlock_t  lock;
} mmap_t;

#define mmap_assert(mmap)           ({ assert(mmap, "No Memory Map"); })
#define mmap_lock(mmap)             ({ mmap_assert(mmap); spin_lock(&(mmap)->lock); })
#define mmap_unlock(mmap)           ({ mmap_assert(mmap); spin_unlock(&(mmap)->lock); })
#define mmap_trylock(mmap)          ({ mmap_assert(mmap); spin_trylock(&(mmap)->lock); })
#define mmap_islocked(mmap)         ({ mmap_assert(mmap); spin_islocked(&(mmap)->lock); })
#define mmap_assert_locked(mmap)    ({ mmap_assert(mmap); spin_assert_locked(&(mmap)->lock); })

int mmap_init(mmap_t *mmap);

/*Dump the Address Space, showing all holes and memory mappings*/
void mmap_dump_list(mmap_t);

/**
 * @brief Allocate a new locked Address Space.
 * @param &mmap_t*
 * @returns 0 on success and -ENOMEM is there are not enough resources
*/
int mmap_alloc(mmap_t **);

/**
 * @brief Map-in a memory region.
 * @param mmap_t *mmap
 * @param vmr_t *r
 * @returns 0 on success, or one of the errors below.
 * @retval -EINVAL if passed a NULL parameter.
 * @retval -EINVAL if 'r' is of size '0'.
 * @retval -EEXIST if part or all of 'r' is already mapped-in.
 */
int mmap_mapin(mmap_t *mmap, vmr_t *region);

/**
 * @brief Same as mmap_mapin()
 *  except that it begins by unmapping any region
 *  that is already mapped-in.
*/
int mmap_forced_mapin(mmap_t *mmap, vmr_t *region);

int mmap_unmap(mmap_t *mmap, uintptr_t start, size_t len);
int mmap_map_region(mmap_t *mmap, uintptr_t addr, size_t len, int prot, int flags, vmr_t **pvmr);

int mmap_contains(mmap_t *mmap, vmr_t *region);

int mmap_remove(mmap_t *mmap, vmr_t *region);

vmr_t *mmap_find(mmap_t *mmap, uintptr_t addr);
vmr_t *mmap_find_exact(mmap_t *mmap, uintptr_t start, uintptr_t end);
vmr_t *mmap_find_vmr_next(mmap_t *mmap, uintptr_t addr, vmr_t **pnext);
vmr_t *mmap_find_vmr_prev(mmap_t *mmap, uintptr_t addr, vmr_t **pprev);
vmr_t *mmap_find_vmr_overlap(mmap_t *mmap, uintptr_t start, uintptr_t end);

/*Begin search at the Start of the Address Space*/
#define __whence_start  0

/*Begin search at the End of the Address Space, walking backwards*/
#define __whence_end    1

int mmap_holesize(mmap_t *mmap, uintptr_t addr, size_t *plen);
int mmap_find_hole(mmap_t *mmap, size_t len, uintptr_t *paddr, int whence);
int mmap_find_holeat(mmap_t *mmap, uintptr_t addr, size_t size, uintptr_t *paddr, int whence);

int mmap_alloc_vmr(mmap_t *mmap, size_t len, int prot, int flags, vmr_t **pvmr);
int mmap_alloc_stack(mmap_t *mmap, size_t len, vmr_t **pstack);
int mmap_vmr_expand(mmap_t *mmap, vmr_t *region, intptr_t incr);

int mmap_protect(mmap_t *mmap, uintptr_t addr, size_t len, int prot);

/// @brief 
/// @param mm 
/// @return 
int mmap_clean(mmap_t *mm);

/// @brief 
/// @param mm 
void mmap_free(mmap_t *mm);

/// @brief 
/// @param dst 
/// @param src 
/// @return 
int mmap_copy(mmap_t *dst, mmap_t *src);

/// @brief 
/// @param mm 
/// @param pclone 
/// @return 
int mmap_clone(mmap_t *mmap, mmap_t **pclone);

/**
 * @brief 
 * 
 */
int mmap_focus(mmap_t *mmap, uintptr_t *ref);

/**
 * @brief 
 * 
 * @param mmap 
 * @param src_argp 
 * @param src_envp 
 * @param argv 
 * @param pargc 
 * @param envv 
 * @return int 
 */
int mmap_argenvcpy(mmap_t *mmap, const char *src_argp[],
                   const char *src_envp[], char **pargv[], int *pargc, char **penvv[]);

/*Is the 'addr' in a hole?*/
#define __ishole(mm, addr)          (mmap_find(mm, addr) == NULL)

#define MAP_PRIVATE                 0x0001
#define MAP_SHARED                  0x0002
#define MAP_DONTEXPAND              0x0004
#define MAP_ANON                    0x0008
#define MAP_ZERO                    0x0010
#define MAP_MAPIN                   0x0020
#define MAP_LOCK                    0x0040
#define MAP_USER                    0x0080
#define MAP_GROWSDOWN               0x0100
/*region is a stack*/
#define MAP_STACK                   (MAP_GROWSDOWN)

/**
 * Map address range as given, 
 * and unmap any overlaping regions previously mapped.
*/
#define MAP_FIXED                   0x1000


#define __flags_locked(flags)       ((flags) & MAP_LOCK)
#define __flags_user(flags)         ((flags) & MAP_USER)
#define __flags_zero(flags)         ((flags) & MAP_ZERO)
#define __flags_anon(flags)         ((flags) & MAP_ANON)
#define __flags_fixed(flags)        ((flags) & MAP_FIXED)
#define __flags_stack(flags)        ((flags) & MAP_STACK)
#define __flags_mapin(flags)        ((flags) & MAP_MAPIN)
#define __flags_shared(flags)       ((flags) & MAP_SHARED)
#define __flags_private(flags)      ((flags) & MAP_PRIVATE)
#define __flags_dontexpand(flags)   ((flags) & MAP_DONTEXPAND)

#define PROT_NONE                   0x0000
#define PROT_READ                   0x0001
#define PROT_WRITE                  0x0002
#define PROT_EXEC                   0x0004

#define PROT_R PROT_READ
#define PROT_W PROT_WRITE
#define PROT_X PROT_EXEC

#define PROT_RW                     (PROT_R | PROT_W)
#define PROT_RX                     (PROT_R | PROT_X)
#define PROT_RWX                    (PROT_R | PROT_W | PROT_X)


#define __prot_exec(prot)           ((prot) & PROT_EXEC)
#define __prot_read(prot)           ((prot) & PROT_READ)
#define __prot_write(prot)          ((prot) & PROT_WRITE)
#define __prot_rx(prot)             (__prot_read(prot) && __prot_exec(prot))
#define __prot_rw(prot)             (__prot_read(prot) && __prot_write(prot))
#define __prot_rwx(prot)            (__prot_rw(prot)   && __prot_exec(prot))

#define VM_EXEC                     0x0001
#define VM_WRITE                    0x0002
#define VM_READ                     0x0004
#define VM_ZERO                     0x0008
#define VM_SHARED                   0x0010
#define VM_FILE                     0x0020
#define VM_GROWSDOWN                0x0100
#define VM_DONTEXPAND               0x0200

#define __vm_mask_exec(flags)       ((flags) &= ~VM_EXEC)
#define __vm_mask_write(flags)      ((flags) &= ~VM_WRITE)
#define __vm_mask_read(flags)       ((flags) &= ~VM_READ)


#define __vmr_mask_exec(vmr)        __vm_mask_exec(vmr->flags)
#define __vmr_mask_write(vmr)       __vm_mask_write(vmr->flags)
#define __vmr_mask_read(vmr)        __vm_mask_read(vmr->flags)

#define __vmr_mask_rwx(vmr)         __vmr_mask_exec(vmr); __vmr_mask_write(vmr); __vmr_mask_read(vmr);

/*Executable*/
#define __vm_exec(flags)            ((flags) & VM_EXEC)

/*Writable*/
#define __vm_write(flags)           ((flags) & VM_WRITE)

/*Readable*/
#define __vm_read(flags)            ((flags) & VM_READ)

/*Readable or Executable*/
#define __vm_rx(flags)              (__vm_read(flags) && __vm_exec(flags))

/*Readable or Writable*/
#define __vm_rw(flags)              (__vm_read(flags) && __vm_write(flags))

/*Readable, Writable or Executable*/
#define __vm_rwx(flags)             (__vm_rw(flags)   && __vm_exec(flags))

#define __vm_zero(flags)            ((flags) & VM_ZERO)

#define __vm_filebacked(flags)      ((flags) & VM_FILE)

/*Sharable*/
#define __vm_shared(flags)          ((flags) & VM_SHARED)

/*Do not expand*/
#define __vm_dontexpand(flags)      ((flags) & VM_DONTEXPAND)

/*Can be expanded*/
#define __vm_can_expand(flags)      (!__vm_dontexpand(flags))

/*Expansion edge grows downwards*/
#define __vm_growsdown(flags)       ((flags) & VM_GROWSDOWN)

/*Expansion edge grows upwards*/
#define __vm_growsup(flags)         (!__vm_growsdown(flags))

#define __vmr_exec(vmr)             __vm_exec(vmr->flags)
#define __vmr_write(vmr)            __vm_write(vmr->flags)
#define __vmr_read(vmr)             __vm_read(vmr->flags)
#define __vmr_rx(vmr)               __vm_rx(vmr->flags)
#define __vmr_rw(vmr)               __vm_rw(vmr->flags)
#define __vmr_rwx(vmr)              __vm_rwx(vmr->flags)
#define __vmr_shared(vmr)           __vm_shared(vmr->flags)
#define __vmr_zero(vmr)             __vm_zero(vmr->flags)
#define __vmr_filebacked(vmr)       (vmr->file && __vm_filebacked(vmr->flags))
#define __vmr_dontexpand(vmr)       __vm_dontexpand(vmr->flags)
#define __vmr_can_expand(vmr)       __vm_can_expand(vmr->flags)
#define __vmr_growsdown(vmr)        __vm_growsdown(vmr->flags)
#define __vmr_growsup(vmr)          __vm_growsup(vmr->flags)

/*Is memory region a stack?*/
#define __isstack(r)                __vmr_growsdown(r)

#define __valid_addr(addr)          ((addr) <= __mmap_limit)

int vmr_alloc(vmr_t **);
void vmr_free(vmr_t *region);
void vmr_dump(vmr_t *region, int index);
int vmr_copy(vmr_t *rdst, vmr_t *rsrc);
int vmr_clone(vmr_t *src, vmr_t **pclone);
int vmr_split(vmr_t *region, uintptr_t addr, vmr_t **pvmr);