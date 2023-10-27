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
    dentry_t *file = NULL;
    printk("Welcome to \e[0;011m'Ginger OS'\e[0m.\n");

    vfs_lookup("/dev/full",     NULL, O_RDWR | O_CREAT, 0660, 0, NULL);
    vfs_lookup("/dev/cpu",      NULL, O_RDWR | O_CREAT | O_DIRECTORY, 0660, 0, NULL);
    vfs_lookup("/dev/pts",      NULL, O_RDWR | O_CREAT | O_DIRECTORY, 0660, 0, NULL);
    vfs_lookup("/dev/zero",     NULL, O_RDWR | O_CREAT, 0660, 0, NULL);
    vfs_lookup("/dev/vfio",     NULL, O_RDWR | O_CREAT | O_DIRECTORY, 0660, 0, NULL);
    vfs_lookup("/dev/snd",      NULL, O_RDWR | O_CREAT | O_DIRECTORY, 0660, 0, NULL);
    vfs_lookup("/dev/random",   NULL, O_RDWR | O_CREAT, 0660, 0, NULL);
    vfs_lookup("/dev/shm",      NULL, O_RDWR | O_CREAT | O_DIRECTORY, 0660, 0, NULL);
    vfs_lookup("/dev/urandom",  NULL, O_RDWR | O_CREAT, 0660, 0, NULL);
    vfs_lookup("/dev/null",     NULL, O_RDWR | O_CREAT, 0660, 0, &file);


    builtin_threads_begin(NULL);
    char b[] = "Hello World\n";

    ilock(file->d_inode);
    iwrite(file->d_inode, 0, b, sizeof b);
    memset(b, 0, sizeof b);
    iread(file->d_inode, 1, b, (sizeof b) -1);
    iunlock(file->d_inode);

    vfs_dirlist("/");

    printk(b);

    loop() thread_join(0, NULL, NULL);
}

