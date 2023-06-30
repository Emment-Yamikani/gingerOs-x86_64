#include <arch/traps.h>
#include <arch/chipset.h>
#include <arch/x86_64/system.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <sys/system.h>
#include <arch/lapic.h>
#include <lib/printk.h>

volatile int intsrc_overide = 0;

#define DATA        0x40

#define COMMAND     0x43
#define COUNTER(i)  (DATA + i)

#define LOB     1
#define HIB     2
#define LOHI    3
#define HZ      100

#define RATEGEN SHL(2, 1) 

void pit_init(void) {
    outb(COMMAND, LOHI | RATEGEN);
    uint16_t counter = 1193182 / HZ;
    outb(COUNTER(0), (uint8_t)counter);
    outb(COUNTER(0), (uint8_t)(counter >> 8));
    ioapic_enable(LEG_PIT, lapic_id());
}