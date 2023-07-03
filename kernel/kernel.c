#include <lib/stddef.h>
#include <lib/stdint.h>
#include <sys/system.h>
#include <boot/boot.h>
#include <lib/printk.h>
#include <arch/x86_64/system.h>
#include <mm/pmm.h>
#include <mm/mm_zone.h>
#include <mm/vmm.h>
#include <mm/kalloc.h>
#include <arch/paging.h>
#include <ds/queue.h>
#include <sys/thread.h>
#include <sync/cond.h>
#include <arch/lapic.h>
#include <arch/traps.h>
#include <dev/dev.h>

void *kmain(void *arg __unused)
{
    int nthread = 0;
    thread_t **tv = NULL;
    INODE file = NULL;

    printk("Welcome to 'Ginger OS'.\n");
    BUILTIN_THREAD_ANOUNCE(__func__);

    vfs_lookup("/tmpfs/file", __UIO("/"), O_CREAT | O_RDONLY, 0, 0777, &file, NULL);

    printk("file: %p\n", file);

    start_builtin_threads(&nthread, &tv);

    loop() {

    }

    return NULL;
}

void garbbage_collector(void) {
    BUILTIN_THREAD_ANOUNCE(__func__);
    loop();
}

BUILTIN_THREAD(garbbage_collector, garbbage_collector, NULL);