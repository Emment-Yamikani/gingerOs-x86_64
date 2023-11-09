#include <arch/cpu.h>
#include <arch/x86_64/msr.h>
#include <arch/x86_64/mmu.h>
#include <arch/x86_64/system.h>
#include <bits/errno.h>
#include <arch/firmware/acpi.h>
#include <sys/system.h>
#include <mm/pmm.h>
#include <sys/thread.h>
#include <lib/string.h>
#include <arch/lapic.h>
#include <mm/kalloc.h>
#include <dev/cga.h>
#include <arch/paging.h>

cpu_t *cpus[MAXNCPU];
static atomic_t ncpu = 1; // '1' because we are starting with BSP
static cpu_t bspcls = {0};
static atomic_t cpus_running = 0;

cpu_t *cpu_getcls(void) {
    return (cpu_t *)rdmsr(IA32_GS_BASE);
}

void cpu_setcls(cpu_t *c) {
    wrmsr(IA32_GS_BASE, (uintptr_t)c);
    wrmsr(IA32_KERNEL_GS_BASE, (uintptr_t)c);
}

void cr0mask(uint64_t bits) {
    uint64_t cr0 = rdcr0();
    cr0 &= (~bits) | 1 | (1 << 31); // never disable paging and protected mode
    wrcr0(cr0);
}

void cr0set(uint64_t bits) {
    uint64_t cr0 = rdcr0();
    cr0 |= bits;
    wrcr0(cr0);
}

uint64_t cr0test(uint64_t bits) {
    uint64_t cr0 = rdcr0();
    return cr0 & bits;
}

uint64_t cr4test(uint64_t bits) {
    uint64_t cr4 = rdcr4();
    return cr4 & bits;
}

void cr4mask(uint64_t bits) {
    uint64_t cr4 = rdcr4();
    cr4 &= (~bits) | CR4_PAE;
    wrcr4(cr4);
}

void cr4set(uint64_t bits) {
    uint64_t cr4 = rdcr4();
    cr4 |= bits;
    wrcr4(cr4);
}

void cpu_get_features(void) {
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;

    cpuid(0, 0, &eax, (uint32_t *)&cpu->vendor[0],
        (uint32_t *)&cpu->vendor[8], (uint32_t *)&cpu->vendor[4]);
    
    cpuid(0x80000008, 0, &eax, &ebx, &ecx, &edx);
    cpu->phys_addrsz = eax & 0xFF;
    cpu->virt_addrsz = (eax >> 8) & 0xFF;

    cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);
    cpu->features |= BTEST(edx, 11) ? CPU_SYSCALL : 0;
    cpu->features |= BTEST(edx, 29) ? CPU_LM : 0;
    cpu->features |= BTEST(edx, 20) ? CPU_XD : 0;

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
    printk("CPU Brand: %s.\n", cpu->brand_string);
    printk("Frequency: %ld Hz\n", cpu->freq);
    printk("Vendor: %s.\n", cpu->vendor);
    printk("Feature0: %X.\n", cpu->features0);
    printk("Feature1: %X.\n", cpu->features1);
    printk("Linear Address Bits: %d-bits.\n", cpu->virt_addrsz);
    printk("Physical Address Bits: %d-bits.\n", cpu->phys_addrsz);
    printk("64-bit Support: %s.\n", cpu->long_mode ? "Yes" : "No");
    printk("XD-bit Support: %s.\n", cpu->execute_disable ? "Yes" : "No");
    printk("SYSCALL/SYSRET Support: %s.\n", cpu->syscall ? "Yes" : "No");
    */
}

void cpu_init(void) {
    disable_caching();
    atomic_inc(&cpus_running);
    memset(cpu, 0, sizeof *cpu);

    idt_init();
    gdt_init();
    cpu_get_features();
    sse_init();

    cpu->flags |= CPU_ONLINE | CPU_64BIT | CPU_ENABLED;
    cpu->flags |= rdmsr(IA32_EFER) & BS(8) ? CPU_64BIT : 0;
    cpu->flags |= BTEST(rdmsr(IA32_APIC_BASE), 8) ? CPU_ISBSP : 0;
}

int is64bit(void) {
    return rdmsr(IA32_EFER) & BS(8) ? 1 : 0;
}

int bsp_init(void) {
    tvinit();
    cpu_setcls(&bspcls);
    cpu_init();
    return 0;
}

void ap_init(void) {
    cpu_setcls(cpus[lapic_id()]);
    cpu_init();
    lapic_init();
    schedule();
    loop();
}

int cpu_count(void) {
    return (int)atomic_read(&ncpu);
}

int cpu_rsel(void) {
    static atomic_t i = 0;
    return (atomic_inc(&i) % ncpu);
}

int cpu_local_id(void) {
    uint32_t a = 0, b = 0, c = 0, d = 0;
    cpuid(0x1, 0, &a, &b, &c, &d);
    return ((b >> 24) & 0xFF);
}

int enumerate_cpus(void) {
    char *entry = NULL;
    acpiMADT_t *MADT = NULL;

    struct {
        uint8_t     type;
        uint8_t     len;
        uint8_t     acpi_id;
        uint8_t     apicID;
        uint32_t    flags;
    } *apic = NULL;

    memset(cpus, 0, sizeof cpus);
    cpus[cpu_id] = cpu;

    if (!(MADT = (acpiMADT_t *)acpi_enumerate("APIC")))
        return -ENOENT;

    entry = (void *)MADT->apics;
    lapic_setaddr(VMA2HI(MADT->lapic_addr));

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

int bootothers(void) {
    uintptr_t *stack = NULL;
    extern char ap_trampoline[];
    uintptr_t v = (uintptr_t)ap_trampoline;
    arch_map_i(v, (uintptr_t)ap_trampoline, PGSZ, VM_KRW);

    for (int i = 0; i < (int)atomic_read(&ncpu); ++i) {
        if (!cpus[i] || !(cpus[i]->flags & CPU_ENABLED) || cpus[i] == cpu)
            continue;

        if (!(stack = (void *)VMA2HI(pmman.get_pages(GFP_NORMAL, 7) + KSTACKSZ)))
            return -ENOMEM;
        *--stack = 0;
        *--stack = (uintptr_t)ap_init;
        *((uintptr_t *)VMA2HI(&ap_trampoline[4032])) = rdcr3();
        *((uintptr_t *)VMA2HI(&ap_trampoline[4040])) = (uintptr_t)stack;
        lapic_startup(cpus[i]->apicID, (uint16_t)((uintptr_t)ap_trampoline));
        while (!(atomic_read(&cpus[i]->flags) & CPU_ONLINE));
    }

    return 0;
}