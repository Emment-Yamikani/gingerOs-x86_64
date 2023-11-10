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
#include <ds/hash.h>
#include <sync/mutex.h>

__noreturn void kthread_main(void) {
    printk("Welcome to \e[0;011m'Ginger OS'\e[0m.\n");
    builtin_threads_begin(NULL);

    char buf[PGSZ];
    dentry_t *file = NULL;

    vfs_lookup("/ramfs/kernel.ld", NULL, O_RDONLY, 0, 0, &file);
    ilock(file->d_inode);
    iread(file->d_inode, 0, buf, sizeof buf);
    iunlock(file->d_inode);
    dclose(file);

    printk(buf);

    loop() thread_join(0, NULL, NULL);
}

