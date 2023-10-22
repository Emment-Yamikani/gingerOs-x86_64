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

void core_start(void);

hash_key_t hash(const char *str) {
    int c = 0;
    hash_key_t hash = 5381;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

int hash_verify(const char *s1, const char *s2) {
    return compare_strings(s1, s2);
}

__noreturn void kthread_main(void) {
    int err = 0;
    int nthread = 0;
    printk("Welcome to \e[0;011m'Ginger OS'\e[0m.\n");
    
    builtin_threads_begin(&nthread, NULL);

    dentry_t *dfile = NULL;
    mode_t mode = S_IRWXU | S_IRWXG | S_IRGRP;

    if ((err = vfs_lookup("/mnt/sda0/bin", NULL, O_CREAT | O_RDWR | O_DIRECTORY, mode, 0, NULL)))
        panic("[PANIC]: %s(), err = %d\n", __func__, err);

    if ((err = vfs_lookup("/mnt/sda0/bin/sh", NULL, O_CREAT | O_RDWR, mode, 0, &dfile)))
        panic("[PANIC]: %s(), err = %d\n", __func__, err);

    ilock(dfile->d_inode);

    iwrite(dfile->d_inode, 0, "\e[0;012mHello World\e[0m\n", 25);
    itruncate(dfile->d_inode);

    char buf[26] = {0};
    iread(dfile->d_inode, 0, buf, sizeof buf);
    iunlock(dfile->d_inode);
    dclose(dfile);

    printk(buf);

    loop() thread_join(0, NULL, NULL);
}