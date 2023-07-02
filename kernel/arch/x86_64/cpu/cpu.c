#include <arch/cpu.h>
#include <arch/x86_64/system.h>
#include <lib/stddef.h>
#include <mm/kalloc.h>
#include <bits/errno.h>
#include <arch/firmware/acpi.h>
#include <lib/string.h>
#include <sync/atomic.h>
#include <lib/printk.h>
#include <arch/paging.h>
#include <arch/lapic.h>
#include <mm/pmm.h>
#include <sys/thread.h>


cpu_t *cpus [MAXNCPU];
static atomic_t ncpu = 1; // '1' because we are starting with BSP

// local cpu data struct for the bsp
static cpu_t bsp;

void cpu_get_features(void)
{
    uint32_t eax, ebx, ecx, edx;

    cpuid(0, 0, &eax, (uint32_t *)&cpu->vendor[0], (uint32_t *)&cpu->vendor[8], (uint32_t *)&cpu->vendor[4]);

    cpuid(0x80000008, 0, (uint32_t *)&cpu->addr_size.raw, &ebx, &ecx, &edx);

    cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);
    
    cpu->features |= BTEST(edx, 11) ? features0_SYSCALL : 0;
    cpu->features |= BTEST(edx, 29) ? features1_LM : 0;
    cpu->features |= BTEST(edx, 20) ? features1_XD : 0;

    cpuid(0x1, 0, (uint32_t *)&cpu->version, &ebx, &ecx, &edx);
    cpu->features |= ((uint64_t)edx << 32) | ecx;

    cpuid(0x80000002, 0, (void *)&cpu->brand_string[0], (void *)&cpu->brand_string[4],
          (void *)&cpu->brand_string[8], (void *)&cpu->brand_string[12]);
    
    cpuid(0x80000003, 0, (void *)&cpu->brand_string[16], (void *)&cpu->brand_string[20],
          (void *)&cpu->brand_string[24], (void *)&cpu->brand_string[28]);
    
    cpuid(0x80000004, 0, (void *)&cpu->brand_string[32], (void *)&cpu->brand_string[36],
          (void *)&cpu->brand_string[40], (void *)&cpu->brand_string[44]);

    cpu->freq = (int)(cpu->brand_string[40] - '0') * 1000 +
                (int)(cpu->brand_string[42] - '0') * 100 +
                (int)(cpu->brand_string[43] - '0') * 10;

    if (!compare_strings(&cpu->brand_string[44], "THz"))
        cpu->freq *= 1000000000;
    else if (!compare_strings(&cpu->brand_string[44], "GHz"))
        cpu->freq *= 1000000;
    else if (!compare_strings(&cpu->brand_string[44], "MHz"))
        cpu->freq *= 1000;

    /*
    printk("freq: %d\n", cpu->brand_string[40]);
    printk("Frequency: %ld %s\n", cpu->freq, &cpu->brand_string[44]);
    printk("Vendor: %s.\n", cpu->vendor);
    printk("Feature0: %X.\n", cpu->features0);
    printk("Feature1: %X.\n", cpu->features1);
    printk("Linear Address Bits: %d-bits.\n", cpu->addr_size.las);
    printk("Physical Address Bits: %d-bits.\n", cpu->addr_size.pas);
    printk("64-bit Support: %s.\n", cpu->long_mode ? "Yes" : "No");
    printk("XD-bit Support: %s.\n", cpu->execute_disable ? "Yes" : "No");
    printk("SYSCALL/SYSRET Support: %s.\n", cpu->syscall ? "Yes" : "No");
    printk("CPU Brand: %s.\n", cpu->brand_string);
    */
}

void cpu_init(cpu_t *c)
{
    disable_caching();
    idt_init();
    gdt_init(c);
    cpu_get_features();
    c->flags |= CPU_ENABLED | BTEST(rdmsr(IA32_APIC_BASE), 8) ? CPU_ISBSP : 0;
    atomic_fetch_or(&c->flags, CPU_ONLINE);
}

int bsp_init(void)
{
    tvinit();
    memset(&bsp, 0, sizeof bsp);
    cpu_init(&bsp);
    return 0;
}

int get_cpu_count(void) {
    return (int)atomic_read(&ncpu);
}

void set_cpu_locale(cpu_t *c) {
    wrmsr(IA32_GS_BASE, (uint64_t)c);
    wrmsr(IA32_KERNEL_GS_BASE, (uint64_t)c);
}

cpu_t *get_cpu_locale(void) { return (cpu_t*)rdmsr(IA32_GS_BASE); }

int cpu_locale_id(void) {
    uint32_t a = 0, b = 0, c = 0, d = 0;
    cpuid(0x1, 0, &a, &b, &c, &d);
    return ((b >> 24) & 0xFF);
}

uintptr_t readgs_base(void) { return rdmsr(IA32_GS_BASE); }

void loadgs_base(uintptr_t base) { wrmsr(IA32_GS_BASE, base); }

int enumerate_cpus(void) {
    char *entry = NULL;
    acpiMADT_t *MADT = NULL;
    extern uint32_t * LAPIC_BASE;
    struct {
        uint8_t type;
        uint8_t len;
        uint8_t acpi_id;
        uint8_t apicID;
        uint32_t flags;
    } *apic = NULL;

    memset(cpus, 0, sizeof cpus);
    cpus[cpu_id] = cpu;

    if (!(MADT = (acpiMADT_t *)acpi_enumerate("APIC")))
        return -ENOENT;

    entry = (void *)MADT->apics;
    LAPIC_BASE = (void *)VMA2HI(MADT->lapic_addr);

    for (; entry && entry < (((char *)MADT) + MADT->madt.length); entry += entry[1]) {
        if (*entry == 0) {
            apic = (void *)entry;
            if ((apic->apicID == cpu_id) || !BTEST(apic->flags, 0))
                continue;
            if (!(cpus[apic->apicID] = (cpu_t *)kcalloc(1, sizeof (cpu_t))))
                panic("Not enough memory to complete operation.\n");

            cpus[apic->apicID]->flags |= CPU_ENABLED;
            cpus[apic->apicID]->apicID = apic->apicID;
            atomic_inc(&ncpu);
        }
    }

    return 0;
}

void ap_start(void) {
    cpu_init(cpus[lapic_id()]);
    lapic_init();
    schedule();
    loop();
}

int bootothers(void) {
    uintptr_t *stack = NULL;
    extern char ap_trampoline[];

    for (int i = 0; i < (int)atomic_read(&ncpu); ++i) {
        if (!cpus[i] || !(cpus[i]->flags & CPU_ENABLED) || cpus[i] == cpu)
            continue;

        if (!(stack = (void *)VMA2HI(pmman.get_pages(GFP_NORMAL, 7) + KSTACKSZ)))
            return -ENOMEM;

        *--stack = (uintptr_t)ap_start;
        *((uintptr_t *)VMA2HI(&ap_trampoline[4032])) = rdcr3();
        *((uintptr_t *)VMA2HI(&ap_trampoline[4040])) = (uintptr_t)stack;
        lapic_startup(cpus[i]->apicID, (uint16_t)((uintptr_t)ap_trampoline));
        while (!(atomic_read(&cpus[i]->flags) & CPU_ONLINE));
    }

    return 0;
}