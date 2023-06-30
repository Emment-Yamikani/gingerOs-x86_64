#include <lib/stdint.h>
#include <lib/stddef.h>
#include <bits/errno.h>
#include <mm/kalloc.h>
#include <sys/system.h>
#include <lib/printk.h>
#include <arch/x86_64/system.h>
#include <arch/firmware/acpi.h>
#include <arch/traps.h>

typedef struct {
    uint32_t flags;
    uint8_t version;
    uint8_t nentries;
    uint8_t ioapic_id;
    uint32_t int_base;
    volatile uint32_t *base_addr;
}ioapic_t;

#define IOAPIC_ENABLED BS(1)

#define REGSEL  (0x00 / 4)
#define IODATA  (0x10 / 4)

#define ID  0x00
#define VER 0x01
#define ARB 0x02
#define REDIR_TBL(i) (0x10 + (i * 2))

#define MASKED  BS(16)
#define LEVEL   BS(15)
#define ACTLOW  BS(13)
#define LOGICAL BS(11)

#define FIXED   0x000
#define LPRIO   0x100
#define SMI     0x200
#define NMI     0x400
#define INIT    0x500

static volatile int nioapic = 0;
static ioapic_t **ioapics = NULL;

static uint32_t read_ioapic(ioapic_t *ioapic, uint32_t reg) {
    if (!ioapic)
        panic("ioapic!!\n");
    ioapic->base_addr[REGSEL] = reg & 0Xff;
    return ioapic->base_addr[IODATA];
}

static void write_ioapic(ioapic_t *ioapic, uint32_t reg, uint32_t data) {
    if (!ioapic)
        panic("ioapic!!\n");
    ioapic->base_addr[REGSEL] = reg & 0Xff;
    ioapic->base_addr[IODATA] = data;
}

void ioapic_enable(int irq, int cpunum) {
    int i = (irq % nioapic);
    write_ioapic(ioapics[i], REDIR_TBL(irq), IRQ(irq));
    write_ioapic(ioapics[i], REDIR_TBL(irq) + 1, cpunum << 24);
}

int ioapic_init(void) {
    char *entry = NULL;
    uint32_t ver_data = 0;
    ioapic_t *ioapic = NULL;
    acpiMADT_t *MADT = NULL;

    if (!(MADT = (acpiMADT_t *)acpi_enumerate("APIC")))
        return -EINVAL;

    // disable 8259A-PICs
    if (BTEST(MADT->flags, 0)) {
        outb(0x22, 0x70);
        outb(0x23, inb(0x23) | 1);
    }

    entry = (char *)MADT->apics;

    // count number of available IO APICs
    for (; entry < ((char *)MADT + MADT->madt.length); entry += entry[1]) {
        if (*entry != 1)
            continue;
        nioapic++;
    }

    if (nioapic > 1)
        printk("WARNING: more than 1 IO APIC detected\n");

    // allocate a pointer vector for the IO APICs
    if (!(ioapics = kcalloc(nioapic, sizeof (ioapic_t *))))
        return -ENOMEM;

    entry = (char *)MADT->apics;

    for (int i = 0; entry < ((char *)MADT + MADT->madt.length); entry += entry[1]) {
        if (*entry != 1)
            continue;
        
        if (!(ioapic = kcalloc(1, sizeof *ioapic)))
            return -ENOMEM;

        ioapics[i++] = ioapic;
        ioapic->ioapic_id = entry[2];
        ioapic->flags = IOAPIC_ENABLED;
        ioapic->int_base = *((uint32_t *)(&entry[8]));
        ioapic->base_addr = (uint32_t *)VMA2HI(*((uint32_t *)(&entry[4])));

        ver_data = read_ioapic(ioapic, VER);
        ioapic->version = (ver_data & 0xFF);
        ioapic->nentries = ((ver_data >> 16) & 0xFF) + 1;

        for (int j = 0; j < ioapic->nentries; ++j) {
            write_ioapic(ioapic, REDIR_TBL(i) + 1, 0);
            write_ioapic(ioapic, REDIR_TBL(i), MASKED | IRQ(i));
        }
    }

    return 0;
}