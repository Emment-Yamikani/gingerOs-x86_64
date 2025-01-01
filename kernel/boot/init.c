#include <boot/boot.h>
#include <bits/errno.h>
#include <sys/system.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <arch/x86_64/mmu.h>
#include <arch/x86_64/cpu.h>
#include <mm/pmm.h>
#include <arch/firmware/acpi.h>
#include <mm/vmm.h>
#include <arch/lapic.h>
#include <arch/paging.h>
#include <arch/chipset.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <dev/dev.h>
#include <dev/fb.h>
#include <dev/console.h>
#include <mm/kalloc.h>
#include <arch/x86_64/ipi.h>

extern __noreturn void kthread_main(void);

#include <dev/cga.h>

int early_init(void) {
    int err = 0;

    if ((err = bsp_init()))
        panic("BSP initialization failed, error: %d\n", err);

    if ((err = vmman.init()))
        panic("Virtual memory initialization failed, error: %d\n", err);

    if ((err = pmman.init()))
        panic("Physical memory initialization failed, error: %d\n", err);

    earlycons_usefb();

    if ((err = acpi_init()))
        panic("Failed to initialize ACPI, error: %d\n", err);

    bootothers();

    pic_init();
    ioapic_init();

    if ((err = dev_init()))
        panic("Failed to start devices, error: %d\n", err);

    if ((err = vfs_init()))
        panic("Failed to initialize VFS!, error: %d\n", err);

    kthread_create(
        NULL, (thread_entry_t)kthread_main,
        NULL, THREAD_CREATE_GROUP |
        THREAD_CREATE_SCHED, NULL
    );

    schedule();
    assert(0, "scheduler returned :(");
    loop();
    return 0;
}