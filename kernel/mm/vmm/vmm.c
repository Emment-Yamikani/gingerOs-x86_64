#include <mm/vmm.h>
#include <mm/pmm.h>
#include <arch/paging.h>
#include <bits/errno.h>
#include <lib/printk.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <lib/string.h>
#include <sys/system.h>
#include <sync/spinlock.h>

#define KVM_DEBUG               0

static atomic_t vmm_online      = 0;
static size_t used_virtual_mmsz = 0;
static spinlock_t *vmm_spinlock = SPINLOCK_NEW();

/// @brief get page size
/// @param  void
/// @return int
int getpagesize(void)           { return PAGESZ; }

typedef struct node {
    uintptr_t       base;   // start of region (zero if free)
    struct  node    *next;  // next region
    struct  node    *prev;  // prev region
    size_t          size;   // size of region in bytes
} node_t;

//size of Kernel in GiB
#define KHEAP_SIZE(sz)          ((GiB(sz)))

#define KHEAPSZ                 KHEAP_SIZE(4) // size of kernel heap.
#define KHEAPBASE               (VMA2HI(GiB(4)))    // kernel heap base address.

#define KHEAP_MAX_NODES         (KHEAPSZ / PAGESZ)                 // maximum blocks that can be address.
#define KHEAP_NODES_ARRAY       ((node_t *)KHEAPBASE)                 // array of memory nodes.
#define KHEAP_NODES_ARRAY_SZ    (KHEAP_MAX_NODES * sizeof(node_t)) // sizeof node array

static node_t *usedvmr_head     = NULL;
static node_t *usedvmr_tail     = NULL;

static node_t *freevmr_head     = NULL;
static node_t *freevmr_tail     = NULL;

static node_t *freenodes_head   = NULL;
static node_t *freenodes_tail   = NULL;

static node_t nodes[KHEAP_MAX_NODES] = {0};

void vmm_dump_node(node_t *node) {
    if (!node)
        panic("No root to dump\n", __FILE__, __LINE__);
    node->prev ? printk("%s:%d: Prev[0x%p, %d KiB]", node->prev->base, node->prev->size / 1024)
               : printk("%s:%d: [Null]");
    printk("%s:%d: <-[0x%p, %d KiB]->", node->base, node->size / 1024);
    node->next ? printk("%s:%d: Next[0x%p, %d KiB]\n", __FILE__, __LINE__, node->next->base, node->next->size / 1024)
               : printk("%s:%d: [Null]\n", __FILE__, __LINE__);
}

static int can_merge_left(node_t *node, node_t *left) {
    assert(node, "no node");
    assert(left, "no left");
    return (left->base + left->size) == node->base;
}

static int can_merge_right(node_t *node, node_t *right) {
    assert(node, "no node");
    assert(right, "no right");
    return (node->base + node->size) == right->base;
}

static int can_merge_both(node_t *left, node_t *node, node_t *right) {
    return can_merge_left(node, left) && can_merge_right(node, right);
}

static node_t *freenodes_get(void) {
    if (!freenodes_head)
        return NULL;
    node_t *node    = freenodes_head;
    freenodes_head  = node->next;
    if (!freenodes_head)
        freenodes_tail = NULL;
    *node = (node_t){0};
    return node;
}

static void freenodes_put(node_t *node) {
    assert(node, "no node");
    assert(node->size, "invalid memory region size");
    assert(node->base >= KHEAPBASE, "invalid memory region base address");
    node->next = NULL;
    node->prev = NULL;

    if (!freenodes_head)
        freenodes_head = node;
    if (freenodes_tail)
        freenodes_tail->next = node;
    node->prev = freenodes_tail;
    freenodes_tail = node;
}

static void merge_left(node_t *node, node_t *left) {
    assert(node, "no node");
    assert(left, "no left");
    if (!can_merge_left(node, left))
        panic("node[%p: %dkib] can't merge with left[%p: %dkib]\n", __FILE__, __LINE__,
              node->base, node->size / 1024, left->base, left->size / 1024);
    left->size += node->size;
    freenodes_put(node);
}

static void merge_right(node_t *node, node_t *right) {
    assert(node, "no node");
    assert(right, "no right");
    if (!can_merge_right(node, right))
        panic("node[%p: %dkib] can't merge with right[%p: %dkib]\n", __FILE__, __LINE__,
              node->base, node->size / 1024, right->base, right->size / 1024);
    right->base = right->base - node->size;
    right->size += node->size;
    freenodes_put(node);
}

static void concatenate(node_t *left, node_t *node, node_t *right) {
    if (!can_merge_both(left, node, right))
        panic("node[%p:%dB] can't merge left[%p:%dB] and right[%p:%dB]\n", __FILE__, __LINE__,
              node->base, node->size, left->base, left->size, right->base, right->size);
    left->size += (node->size + right->size);
    if (right->next)
        right->next->prev = left;
    left->next = right->next;
    if (!right->next)
        freevmr_tail = left;
    freenodes_put(node);
    freenodes_put(right);
}

static void freevmr_linkto_left(node_t *node, node_t *left) {
    assert(node, "no node");
    assert(left, "no left");

    node->next = left->next;
    if (left->next)
        left->next->prev = node;
    else {
        freevmr_tail = node;
        node->next = NULL;
    }
    left->next = node;
    node->prev = left;
}

static void freevmr_linkto_right(node_t *node, node_t *right) {
    assert(node, "no node");
    assert(right, "no right");

    node->prev = right->prev;
    if (right->prev)
        right->prev->next = node;
    else {
        freevmr_head = node;
        node->prev = NULL;
    }
    right->prev = node;
    node->next = right;
}

static void freevmr_put(node_t *node) {
    assert(node, "no node");
    assert(node->size, "invalid memory region size");
    assert(node->base >= KHEAPBASE, "invalid memory region base address");

    node_t *tmp = NULL, *right = freevmr_head, *left = NULL;

    node->prev = NULL;
    node->next = NULL;

#if KVM_DEBUG
    printk("%s:%d: putting back: [0x%p, %dKiB] return to 0x%p\n", __FILE__, __LINE__, node->base, node->size / 1024, __retaddr(0));
#endif

    if (!freevmr_head) {
        freevmr_head = node;
        freevmr_tail = node;
        return;
    }

    for (tmp = right; tmp && (right->base < node->base); tmp = right = tmp->next) {
        left = right;
    }

    if (left && right) {
#if KVM_DEBUG
        printk("%s:%d: left and right\n", __FILE__, __LINE__);
#endif
        if (can_merge_both(left, node, right)) {
#if KVM_DEBUG
            printk("%s:%d: can merge with both L[%p : %dkib] : [%p : %dkib] : R[%p : %dkib]\n", __FILE__, __LINE__,
                   left->base, left->size / 1024, node->base, node->size / 1024, right->base, right->size / 1024);
#endif
            concatenate(left, node, right);
        }
        else if (can_merge_left(node, left)) {
#if KVM_DEBUG
            printk("%s:%d: can merge with left L[%p : %dkib] : [%p : %dkib] : R[%p : %dkib]\n", __FILE__, __LINE__,
                   left->base, left->size / 1024, node->base, node->size / 1024, right->base, right->size / 1024);
#endif
            merge_left(node, left);
        }
        else if (can_merge_right(node, right)) {
#if KVM_DEBUG
            printk("%s:%d: can merge with right L[%p : %dkib] : [%p : %dkib] : R[%p : %dkib]\n", __FILE__, __LINE__,
                   left->base, left->size / 1024, node->base, node->size / 1024, right->base, right->size / 1024);
#endif
            merge_right(node, right);
        }
        else {
#if KVM_DEBUG
            printk("%s:%d: can not merge with any L[%p : %dkib] : [%p : %dkib] : R[%p : %dkib]\n", __FILE__, __LINE__,
                   left->base, left->size / 1024, node->base, node->size / 1024, right->base, right->size / 1024);
#endif
            // freevmr_linkto_left(node, left);
            // freevmr_linkto_right(node, right);

            node->prev = left;
            node->next = right;
            
            left->next = node;
            right->prev = node;

        }
    }
    else if (left) {
#if KVM_DEBUG
        printk("%s:%d: only has left L[%p : %dkib] : [%p : %dkib]\n", __FILE__, __LINE__,
               left->base, left->size / 1024, node->base, node->size / 1024);
#endif
        if (can_merge_left(node, left)) {
#if KVM_DEBUG
            printk("%s:%d: can merge left L[%p : %dkib] : [%p : %dkib]\n", __FILE__, __LINE__,
                   left->base, left->size / 1024, node->base, node->size / 1024);
#endif
            merge_left(node, left);
        }
        else {
#if KVM_DEBUG
            printk("%s:%d: can not merge left L[%p : %dkib] : [%p : %dkib]\n", __FILE__, __LINE__,
                   left->base, left->size / 1024, node->base, node->size / 1024);
#endif
            freevmr_linkto_left(node, left);
        }
    }
    else if (right) {
#if KVM_DEBUG
        printk("%s:%d: only has right [%p : %dkib] : R[%p : %dkib]\n", __FILE__, __LINE__,
               node->base, node->size / 1024, right->base, right->size / 1024);
#endif
        if (can_merge_right(node, right)) {
#if KVM_DEBUG
            printk("%s:%d: can merge right [%p : %dkib] : R[%p : %dkib]\n", __FILE__, __LINE__,
                   node->base, node->size / 1024, right->base, right->size / 1024);
#endif
            merge_right(node, right);
        }
        else {
#if KVM_DEBUG
            printk("%s:%d: can not merge right [%p : %dkib] : R[%p : %dkib]\n", __FILE__, __LINE__,
                   node->base, node->size / 1024, right->base, right->size / 1024);
#endif
            freevmr_linkto_right(node, right);
        }
    }
    else
        panic("impossible constraint\n", __FILE__, __LINE__);
}

static node_t *freevmr_get(size_t sz) {
    assert(sz, "invalid memory size request");
    assert(!(sz & PAGEMASK), "invalid size, must be page aligned");
    node_t *node = freevmr_head, *next = NULL, *prev = NULL;

    if (!node)
        return NULL;

#if KVM_DEBUG
    printk("%s:%d: requested: %d\n", __FILE__, __LINE__, sz / 1024);
#endif

    for (; node; node = node->next) {
        // vmm_dump_node(node);
        if (node->size >= sz)
            break;
    }

    if (node) {
        next = node->next;
        prev = node->prev;

        node->next = NULL;
        node->prev = NULL;

        if (prev)
            prev->next = next;

        if (freevmr_head == node) {
            freevmr_head = next;
            if (freevmr_head)
                freevmr_head->prev = NULL;
        }

        if (next)
            next->prev = prev;

        if (freevmr_tail == node) {
            freevmr_tail = next;
            if (freevmr_tail)
                freevmr_tail->next = NULL;
        }
    }

    return node;
}

static void usedvmr_put(node_t *node) {
    assert(node, "no node");
    assert(node->size, "invalid memory region size");
    assert(node->base >= KHEAPBASE, "invalid memory region base address");
    if (!usedvmr_head)
        usedvmr_head = node;
    if (usedvmr_tail)
        usedvmr_tail->next = node;
    node->next = NULL;
    node->prev = usedvmr_tail;
    usedvmr_tail = node;
}

static node_t *usedvmr_lookup(uintptr_t base) {
    assert(base, "no memory region base address specified");
    node_t *node = usedvmr_head;

    if (!node)
        return NULL;

    for (; node; node = node->next)
        if (node->base == base)
            break;

    if (node) {
        if (node->prev)
            node->prev->next = node->next;

        if (node->next)
            node->next->prev = node->prev;

        if (usedvmr_head == node)
            usedvmr_head = node->next;

        if (usedvmr_tail == node)
            usedvmr_tail = node->prev;

        node->next = NULL;
        node->prev = NULL;
    }
    return node;
}

static uintptr_t vmm_alloc(size_t sz) {
    uintptr_t addr = 0;
    node_t *split = NULL, *node = NULL;

#if KVM_DEBUG
    printk("%s:%d: alocating %dKib @ %p\n", __FILE__, __LINE__, sz / 1024, addr);
#endif

    if (!atomic_read(&vmm_online))
        vmman.init();

    spin_lock(vmm_spinlock);

    split = freevmr_get(sz);

    if (!split) {
#if KVM_DEBUG
        printk("%s:%d: couldn't allocate %dkib, no free virtual memory available\n", __FILE__, __LINE__, sz / 1024);
#endif
        spin_unlock(vmm_spinlock);
#if KVM_DEBUG
        printk("%s:%d: No split\n", __FILE__, __LINE__);
#endif
        return 0;
    }

    if (split->size > sz) {
        node = freenodes_get();
        assert(node, "no free nodes left");
        node->base = split->base;
        node->size = sz;

#if KVM_DEBUG
        printk("%s:%d: Region too big: [0x%p, %d KiB], after split: %d KiB\n", __FILE__, __LINE__, split->base, split->size / 1024, (split->size - sz) / 1024);
#endif

        split->base += sz;
        split->size -= sz;
        freevmr_put(split);
    }
    else if (split->size < sz)
        panic("impossible contraint with size\n", __FILE__, __LINE__);
    else
        node = split;

    used_virtual_mmsz += sz;

    addr = node->base;
    usedvmr_put(node);

#if KVM_DEBUG
    printk("%s:%d: alocated %dKib @ %p\n", __FILE__, __LINE__, sz / 1024, addr);
#endif

    // printk("%s:%d: node: %p, data: %p, end: %p, size: %d kiB\n", __FILE__, __LINE__, node, addr, node->size + addr, node->size / 1024);
    spin_unlock(vmm_spinlock);
    return addr;
}

static void vmm_free(uintptr_t base) {
    node_t *node = NULL;
    assert(base, "no memory region base address specified");

    spin_lock(vmm_spinlock);

    if (!(node = usedvmr_lookup(base)))
        panic("%p was not allocated before\n", __FILE__, __LINE__, base);

#if KVM_DEBUG
    printk("%s:%d: freeing %p\n", __FILE__, __LINE__, base);
#endif

    used_virtual_mmsz -= node->size;
    freevmr_put(node);
    // vmm_dump_list(node);
#if KVM_DEBUG
    printk("%s:%d: done freeing\n", __FILE__, __LINE__);
#endif

#if KVM_DEBUG
    printk("%s:%d: freed %dkib @ %p\n", __FILE__, __LINE__, node->size / 1024, node->base);
#endif

    spin_unlock(vmm_spinlock);
}

static size_t vmm_getfreesize(void) {
    spin_lock(vmm_spinlock);
    size_t size = (((size_t)KHEAPSZ) - used_virtual_mmsz);
    spin_unlock(vmm_spinlock);
    return size / 1024;
}

static size_t vmm_getinuse(void) {
    spin_lock(vmm_spinlock);
    size_t size = used_virtual_mmsz;
    spin_unlock(vmm_spinlock);
    return size / 1024;
}

static int vmm_init(void) {
    size_t i = 0;
    memset(nodes, 0, sizeof nodes);
    for (; i < (KHEAP_MAX_NODES - 1); ++i) {
        nodes[i].prev = nodes + (i - 1);
        nodes[i].next = (nodes + (i + 1));
    }

    nodes[0].prev = NULL;
    nodes[i].next = NULL;
    nodes[i].prev = &nodes[i - 1];

    freenodes_head = nodes;
    freenodes_tail = &nodes[i];

    node_t *node = NULL;
    *(node = freenodes_get()) = (node_t){
        .base = KHEAPBASE,
        .size = KHEAPSZ};

    freevmr_put(node);
    atomic_write(&vmm_online, 1);

    return 0;
}

int vmm_active(void) {
    return atomic_read(&vmm_online);
}

void memory_usage(void) {
    printk(
        "\n\t\t\t\e[025453;06mMEMORY USAGE INFO\e[0m\n"
        "\t\t\t\e[025453;015mPhysical Memory\e[0m\n"
        "Free  : \e[025453;012m%8.1F MiB\e[0m\n"
        "In use: \e[025453;04m%8.1F MiB\e[0m\n\n"
        "\t\t\t\e[025453;015mVirtual Memory\e[0m\n"
        "Free  : \e[025453;012m%8.1F MiB\e[0m\n"
        "In use: \e[025453;04m%8.1F MiB\e[0m\n\n",
        (double)pmman.mem_free() / KiB(1),
        (double)pmman.mem_used() / KiB(1),
        (double)vmman.getfreesize() / KiB(1),
        (double)vmman.getinuse() / KiB(1)
    );
}

struct vmman vmman = {
    .free        = vmm_free,
    .init        = vmm_init,
    .alloc       = vmm_alloc,
    .getinuse    = vmm_getinuse,
    .getfreesize = vmm_getfreesize,
};