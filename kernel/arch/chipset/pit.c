#include <arch/traps.h>
#include <arch/chipset.h>
#include <arch/x86_64/system.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <sys/system.h>
#include <arch/lapic.h>
#include <lib/printk.h>
#include <ginger/jiffies.h>

volatile int intsrc_overide = 0;

#define DATAPORT    0x40

#define CMDPORT     0x43
#define COUNTER(i)  (DATAPORT + i)

#define CH(i)   SHL(i, 6)

#define LOB     1
#define HIB     2
#define LOHI    3
#define HZ      SYS_HZ

#define MODE(x) SHL(x, 1)
#define RATEGEN MODE(2)
#define ONESHOT MODE(1)

void pit_init(void) {
    outb(CMDPORT, LOHI | RATEGEN);
    uint16_t counter = 1193182 / HZ;
    outb(COUNTER(0), (uint8_t)counter);
    inb(0x60);
    outb(COUNTER(0), (uint8_t)(counter >> 8));
    ioapic_enable(LEG_PIT, lapic_id());
}

void pit_intr(void) {
    jiffies_update();
}

void pit_wait(double s) {
    uint8_t data = 0;
    uint16_t freq = (uint16_t) ((double) (1 / s));
    uint16_t counter = 1193182 / freq;

    data = (inb(0x61) & 0xfd) | 1;
    outb(0x61, data);

    outb(CMDPORT, CH(2) | LOHI | ONESHOT);
    outb(COUNTER(2), (uint8_t)counter);
    inb(0x60);
    outb(COUNTER(2), (uint8_t)(counter >> 8));

    data = inb(0x61) & 0xfe;
    outb(0x61, data);
    outb(0x61, data | 1);
    while (!(inb(0x61) & 0x20));
}