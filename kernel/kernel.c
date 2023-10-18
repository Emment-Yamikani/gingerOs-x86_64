#include <arch/x86_64/system.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <sys/system.h>
#include <boot/boot.h>
#include <ginger/jiffies.h>
#include <lib/printk.h>
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
#include <sys/sleep.h>
#include <sys/_signal.h>
#include <dev/hpet.h>
#include <modules/module.h>
#include <fs/tmpfs.h>
#include <fs/fs.h>
#include <ds/stack.h>
#include <lib/ctype.h>

void core_start(void);

__noreturn void kthread_main(void) {
    int err = 0;
    int nthread = 0;
    printk("Welcome to 'Ginger OS'.\n");
    
    builtin_threads_begin(&nthread, NULL);

    inode_t *folder = NULL;
    mode_t mode = S_IRWXU | S_IRWXG | S_IRGRP;

    memory_usage();

    if ((err = vfs_lookup("/mnt/folder", NULL, O_CREAT | O_RDWR | O_DIRECTORY, mode, 0, &folder, NULL)))
        panic("[PANIC]: %s(), err = %d\n", __func__, err);

    loop() thread_join(0, NULL, NULL);
}