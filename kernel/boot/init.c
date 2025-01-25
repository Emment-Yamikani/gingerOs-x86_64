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

    assert_eq(err = bsp_init(), 0, "BSP initialization failed, error: %d\n", err);

    assert_eq(err = vmman.init(), 0, "Virtual memory initialization failed, error: %d\n", err);

    assert_eq(err = pmman.init(), 0, "Physical memory initialization failed, error: %d\n", err);

    earlycons_usefb();

    assert_eq(err = acpi_init(), 0, "Failed to initialize ACPI, error: %d\n", err);

    bootothers();

    pic_init();
    ioapic_init();

    assert_eq(err = dev_init(), 0, "Failed to start devices, error: %d\n", err);

    assert_eq(err = vfs_init(), 0, "Failed to initialize VFS!, error: %d\n", err);

    assert_eq(err = kthread_create(
        NULL, (thread_entry_t)kthread_main,
        NULL, THREAD_CREATE_GROUP |
        THREAD_CREATE_SCHED, NULL
    ), 0, "Failed to create main kernel thread, error: %d\n", err);

    scheduler();
    assert(0, "scheduler returned :(");
    loop() { cli(); hlt(); } // no where to go from here.
}