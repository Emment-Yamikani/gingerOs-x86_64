#include <dev/dev.h>
#include <modules/module.h>
#include <bits/errno.h>
#include <lib/printk.h>
#include <mm/kalloc.h>
#include <lib/string.h>
#include <bits/errno.h>
#include <arch/chipset.h>
#include <lib/stdint.h>
#include <arch/traps.h>
#include <arch/firmware/acpi.h>
#include <ginger/jiffies.h>

typedef union { struct {
    uint64_t revID : 8;
    uint64_t num_tim_cap : 5;
    uint64_t count_size_cap : 1;
    uint64_t resvd : 1;
    uint64_t legacy_rt : 1;
    uint64_t vendorID : 16;
    uint64_t count_clk_tick : 32;
}; uint64_t raw;
} __packed general_capID_t;

typedef union {
    struct {
        uint64_t enable_cnf : 1;
        uint64_t legacy_rt_cnf : 1;
    }; uint64_t raw;
} __packed general_config_t;

typedef union { struct {
        uint64_t resvd0 : 1;
        uint64_t level_triggered_cnf : 1;       // generate level triggered interrupts.
        uint64_t intena_cnf: 1;                 // enable interrupts
        uint64_t periodic_enable_cnf : 1;       // generate(allow) periodic interrupts
        uint64_t periodic_cap : 1;              // timer able to generate periodic interrupts?
        uint64_t size_cap : 1;                  // size of the timer. 1=64bit and 0=32bit.
        uint64_t value_set_cnf : 1;             // allow software to directly write the timers accumulator(IN PERIODIC MODE ONLY)
        uint64_t resvd1: 1;
        uint64_t mode32_cnf : 1;                // use the timer in 32 bit mode.
        uint64_t int_rt_cnf : 5;                // IOAPIC int route.
        uint64_t fsb_en_cnf: 1;                 // FSB delivery enable?
        uint64_t fsb_del_cap : 1;               // front side bus delivery capable?
        uint64_t resvd2: 16;
        uint64_t int_rt_cap : 32;               // which interrupt line on IOAPIC can this timer be routed?
    };
    uint64_t raw;
}__packed timer_config_t;

typedef struct {
    acpiSDT_t HDR;
    uint32_t timer_blockID;
    struct {
        uint8_t addr_ID;
        uint8_t register_width;
        uint8_t register_offset;
        uint8_t resvd;
        uint64_t timer_block_addr;
    } __packed base_addr;
    uint8_t hpet_number;
    uint16_t min_clk_tick;
    struct {
        uint8_t no_guarantee : 1;
        uint8_t _4kib : 1;
        uint8_t _64kib : 1;
        uint8_t resvd : 4;
    } __packed page_protection;
} __packed acpi_hpet_t;

static volatile acpi_hpet_t *acpi_hpet = NULL;

#define REG_CAP     (0x0 / 8)
#define REG_CONF    (0x10 / 8)
#define REG_INTS    (0x20 / 8)
#define REG_COUNTER (0xF0 / 8)

#define TIMER(n)      (0x100 + (n* 0x20))
#define COMP_CAP(n)   ((TIMER(n) + 0) / 8)
#define COMP_VALUE(n) ((TIMER(n) + 8) / 8)
#define COMP_FSB(n)   ((TIMER(n) + 16) / 8)

__unused static volatile uint64_t *hpet = NULL;

uint64_t hpet_read(int reg) {
    if (!hpet)
        return 0;
    return hpet[reg];
}

void hpet_write(int reg, uint64_t data) {
    if (!hpet)
        return;
    hpet[reg] = data;
}

void hpet_enable(void) {
    hpet_write(REG_CONF, (hpet_read(REG_CONF) | 1));
}

void hpet_disable(void) {
    hpet_write(REG_CONF, (hpet_read(REG_CONF) & ~1));
}

void hpet_eoi(int i) {
    hpet_write(REG_INTS, hpet_read(REG_INTS) | (1 << i));
}


static int ntimers = 0;
static long freq   = 0;
static size_t period_fs = 0; // main counter period in fs
static size_t period_ns = 0; // main counter period in ns
static size_t tick_per_100ns = 0; // number of ticks in 100 ns

int hpet_init(void) {
    int err = -ENOENT;
    printk("Initializing High Precision Event Timer (HPET)...\n");

    if (!(acpi_hpet = (acpi_hpet_t *)acpi_enumerate("HPET")))
        return err;
    
    hpet = (void *)acpi_hpet->base_addr.timer_block_addr;

    general_capID_t cap = (general_capID_t)hpet_read(0);

    ntimers = cap.num_tim_cap + 1;
    period_fs = cap.count_clk_tick;
    period_ns = period_fs / 1000000;
    tick_per_100ns = 100000000 / period_fs;
    freq = 1000000000000000 / cap.count_clk_tick;
    size_t ticks = 1000000000000000 / SYS_HZ; // HZ_TO_ns(SYS_HZ) / period_ns;

    hpet_disable();

    hpet_write(REG_COUNTER, 0);

    timer_config_t tcnf = (timer_config_t)hpet_read(COMP_CAP(0));

    tcnf.mode32_cnf = 0;
    tcnf.intena_cnf = 1;
    tcnf.value_set_cnf = 1;
    tcnf.int_rt_cnf = HPET;
    tcnf.level_triggered_cnf = 1;
    tcnf.periodic_enable_cnf = 1;

    hpet_write(COMP_CAP(0), tcnf.raw);
    hpet_write(COMP_VALUE(0), ticks);

    hpet_enable();

    ioapic_enable(HPET, 0);
    return 0;
}



void hpet_intr(void) {
    uint64_t ints = hpet_read(REG_INTS);
    for (int t = 0; t < ntimers; ++t) {
        if (BTEST(ints, t))
            hpet_write(REG_INTS, ints | BS(t));
        if (t == 0)
            jiffies_update();
    }
}

MODULE_INIT(HPET, hpet_init, NULL);