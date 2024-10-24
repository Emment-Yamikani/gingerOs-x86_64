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

typedef struct node_t {
    struct node_t *prev;
    uintptr_t     base;
    size_t        size;
    struct node_t *next;
} node_t;

#define node_assert(node)       ({ assert(node, "No vm node."); })
#define node_get_len(node)      ({ node_assert(node); (node)->size; })
#define node_get_start(node)    ({ node_assert(node); (node)->start; })

#define KHEAPSIZE               (MiB(128))
#define KHEAPBASE               (VMA2HI(GiB(4)))
#define NNODES                  (KHEAPSIZE / 4096)

static      atomic_t    initialized     = 0;
static      size_t      used_memsz      = 0;
static      node_t      *free_node_list = NULL;
static      node_t      *usedvmr_list   = NULL;
static      node_t      *freevmr_list   = NULL;
static      node_t      nodes[NNODES];

static      spinlock_t  *vmlk           = &SPINLOCK_INIT();

#define vm_lock()               ({ spin_lock(vmlk); })
#define vm_unlock()             ({ spin_unlock(vmlk); })
#define vm_islocked()           ({ spin_islocked(vmlk); })
#define vm_assert_locked()      ({ spin_assert_locked(vmlk); })


static void node_dump(node_t *node, size_t i) {
    if (node->prev && node->next) {
        printk("%8.8ld: [%16p:%8ld KiB]<-[%16p:%8ld KiB]->[%16p:%8ld KiB]\n", i,
            (void *)node->prev->base, node->prev->size / KiB(1),
            (void *)node->base, node->size / KiB(1),
            (void *)node->next->base, node->next->size / KiB(1)
        );
    } else if (node->prev) {
        printk("%8.8ld: [%16p:%8ld KiB]<-[%16p:%8ld KiB]->[null]\n", i,
            (void *)node->prev->base, node->prev->size / KiB(1),
            (void *)node->base, node->size / KiB(1)
        );
    } else if (node->next) {
        printk("%8.8ld: [null]<-[%16p:%8ld KiB]->[%16p:%8ld KiB]\n", i,
            (void *)node->base, node->size / KiB(1),
            (void *)node->next->base, node->next->size / KiB(1)
        );
    } else {
        printk("%8.8ld: [null]<-[%16p:%8ld KiB]->[null]\n", i,
            (void *)node->base, node->size / KiB(1)
        );
    }
}

static void dump_list(node_t *list, const char *list_name) {
    size_t i = 0;
    printk("virtual memory regions:[%s]\n-START-\n", list_name);
    forlinked(node, list, node->next)
        node_dump(node, i++);
    printk("-END(%ld nodes)-\n\n", i);
}

void dump_free_node_list(void) {
    dump_list(free_node_list, "FREE NODES");
}

void dump_freevmr_list(void) {
    dump_list(freevmr_list, "FREE VMR");
}

void dump_usedvmr_list(void) {
    dump_list(usedvmr_list, "USED VMR");
}

void dump_all_lists(void) {
    dump_free_node_list();
    dump_freevmr_list();
    dump_usedvmr_list();
}

static node_t *free_node_get(void) {
    node_t *node = NULL;

    vm_assert_locked();

    if (free_node_list == NULL)
        return NULL;

    node  = free_node_list;

    if ((free_node_list  = node->next))
        free_node_list->prev = NULL;

    *node = (node_t) {0};
    return node;
}

static void free_node_put(node_t *node) {
    node_assert(node);
    vm_assert_locked();
    *node       = (node_t) {0};
    node->next  = free_node_list;
    if (free_node_list)
        free_node_list->prev = node;
    free_node_list  = node;
}

static int can_merge_prev(node_t *node, node_t *left) {
    node_assert(node);
    vm_assert_locked();
    if (left == NULL)
        return 0;
    return (left->base + left->size) == node->base;
}

static int can_merge_next(node_t *node, node_t *right) {
    node_assert(node);
    vm_assert_locked();
    if (right == NULL)
        return 0;
    return (node->base + node->size) == right->base;
}

static void usedvmr_put(node_t *node) {
    node_t *tail = NULL;
    node_assert(node);
    vm_assert_locked();

    node->next = NULL;
    node->prev = NULL;

    forlinked(right, usedvmr_list, right->next) {
        if (node->base < right->base) {
            node->next = right;
            if (right->prev) {
                right->prev->next = node;
            } else usedvmr_list = node;
            node->prev  = right->prev;
            right->prev = node;
            return;
        }
        tail = right;
    }

    if (tail) {
        tail->next = node;
        node->prev = tail;
    } else usedvmr_list = node;
}

static int usedvmr_find(uintptr_t base, node_t **ppn) {
    vm_assert_locked();

    if (base == 0 || ppn == NULL)
        return -EINVAL;
    
    forlinked(node, usedvmr_list, node->next) {
        if (base == node->base) {
            *ppn = node;
            return 0;
        }
    }

    return -ENOENT;
}

static void freevmr_put(node_t *node) {
    node_t *left = NULL, *right = freevmr_list;

    node_assert(node);
    vm_assert_locked();

    node->next = NULL;
    node->prev = NULL;

    // Traverse the list to find the correct insertion point based on node->base
    while (right && right->base < node->base) {
        left = right;
        right = right->next;
    }

    // Check if we can merge with the left node
    if (left && can_merge_prev(node, left)) {
        left->size += node->size;
        // Try to merge left with the next (right) node if possible
        if (right && can_merge_next(left, right)) {
            left->size += right->size;
            left->next = right->next;
            if (right->next) {
                right->next->prev = left;
            }
            free_node_put(right);
        }
        free_node_put(node);
        return;
    }

    // Check if we can merge with the right node
    if (right && can_merge_next(node, right)) {
        right->size += node->size;
        right->base -= node->size;
        if (left) {
            left->next = right;
        } else {
            freevmr_list = right;
        }
        right->prev = left;
        free_node_put(node);
        return;
    }

    // If no merge happened, insert the node between left and right
    node->next = right;
    node->prev = left;

    if (right) {
        right->prev = node;
    }

    if (left) {
        left->next = node;
    } else {
        freevmr_list = node;  // New node becomes the head if left is NULL
    }
}

static int freevmr_get(size_t size, node_t **ppn) {
    vm_assert_locked();

    if (size == 0 || ppn == NULL)
        return -EINVAL;

    forlinked(node, freevmr_list, node->next) {
        if (node->size >= size) {
            *ppn = node;
            return 0;
        }
    }
    return -ENOMEM;
}

static int vmm_init(void) {
    node_t *node = NULL;

    if (atomic_read(&initialized))
        return 0;
    
    vm_lock();

    memset(nodes, 0, sizeof nodes);

    for (node  = nodes; node < &nodes[NNODES]; node++) {
        if (node > nodes)
            node->prev = node - 1;
        if ((node + 1) < &nodes[NNODES])
            node->next = node + 1;
    }

    free_node_list = nodes;

    *(node = free_node_get()) = (node_t) {
        .base = KHEAPBASE,
        .size  = KHEAPSIZE,
    };

    freevmr_put(node);
    
    vm_unlock();
    
    atomic_write(&initialized, 1);
    return 0;
}

static int alloc(size_t size, void **ppv) {
    int     err = 0;
    node_t *node = NULL, *split = NULL;
    node_t *prev = NULL, *next = NULL;

    if (!atomic_read(&initialized))
        vmm_init();

    vm_lock();

    if ((err = freevmr_get(size, &split))) {
        vm_unlock();
        return err;
    }

    if (split->size > size) {
        node_assert(node = free_node_get());
        node->base = split->base;
        split->base += size;
        split->size -= size;
        node->size = size;
        *ppv = (void *)node->base;
        usedvmr_put(node);
    } else if (split->size == size) {
        next = split->next;
        prev = split->prev;

        if (next)
            next->prev = prev;
        if (prev)
            prev->next = next;
        else freevmr_list = next;
        *ppv = (void *)split->base;
        usedvmr_put(split);
    } else assert(0, "Failed to get correct sized vmr_node");

    used_memsz += size;

    vm_unlock();
    return 0;
}

static void free(void *addr) {
    int err = 0;
    node_t *node = NULL, *right = NULL;

    assert(addr, "cannot free nullptr");

    vm_lock();

    if ((err = usedvmr_find((uintptr_t)addr, &node))) {
        vm_unlock();
        return;
    }

    right = node->next;

    if (node->prev)
        node->prev->next = right;
    else usedvmr_list = right;

    if (right)
        right->prev = node->prev;

    used_memsz -= node->size;

    freevmr_put(node);

    vm_unlock();
}

void vmm_free(uintptr_t addr) {
    free((void *)addr);
}

uintptr_t vmm_alloc(size_t size) {
    uintptr_t   addr    = 0;
    alloc(size, (void **)&addr);
    return addr;
}

size_t vmm_getinuse() {
    return used_memsz;
}

size_t vmm_getfreesize() {
    return KHEAPSIZE - used_memsz;
}

int vmm_active(void) {
    return atomic_read(&initialized);
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

int getpagesize(void) {
    return PGSZ;
}


struct vmman vmman = {
    .init        = vmm_init,
    .free        = vmm_free,
    .alloc       = vmm_alloc,
    .getinuse    = vmm_getinuse,
    .getfreesize = vmm_getfreesize,
};