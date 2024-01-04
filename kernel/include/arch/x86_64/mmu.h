#pragma once

#include <lib/stdint.h>
#include <sys/system.h>
#include <lib/types.h>

#define LF_IF           0x202

#define DPL_KRN         (0)
#define DPL_USR         (3)

#define SEG_NULL        0
#define SEG_KCODE64     1
#define SEG_KDATA64     2
#define SEG_UCODE64     3
#define SEG_UDATA64     4
#define SEG_TSS64       5
//#define SEG_KCPU64      5

#define SEG_DATA        (0x2)
#define SEG_CODE        (0xA)
#define TSS_SEG         (0x9)
#define SEG_CODEDATA    (BS(0))
#define SEG_PRESENT     (BS(3))
#define SEG_LONG        (BS(5))
#define SEG_DB          (BS(6))
#define SEG_GRAN        (BS(7))
#define SEG_DPL(dpl)    (SHL(dpl, 1))

#define DATA_SEG(dpl)   (SEG_GRAN | SEG_DB | SEG_PRESENT | SEG_DPL(dpl) | SEG_CODEDATA)

#define KCODE_SEG       (SEG_GRAN | SEG_LONG | SEG_PRESENT | SEG_DPL(DPL_KRN) | SEG_CODEDATA)
#define KDATA_SEG       (DATA_SEG(DPL_KRN))

#define UCODE_SEG64     (SEG_GRAN | SEG_LONG | SEG_PRESENT | SEG_DPL(DPL_USR) | SEG_CODEDATA)
#define UCODE_SEG32     DATA_SEG(DPL_USR)
#define UDATA_SEG       DATA_SEG(DPL_USR)

typedef struct {
    uint32_t rsvd0;
    uintptr_t rsp0;
    uintptr_t rsp1;
    uintptr_t rsp2;
    uintptr_t rsvd1;
    uintptr_t ist1;
    uintptr_t ist2;
    uintptr_t ist3;
    uintptr_t ist4;
    uintptr_t ist5;
    uintptr_t ist6;
    uintptr_t ist7;
    uintptr_t rsvd2;
    uint32_t rsvd3 : 16;
    uint32_t iomap : 16;
} __packed tss_t;

typedef struct {
    uint16_t limit0;
    uint16_t base0;
    uint16_t base1 : 8;
    uint16_t accel : 4;
    uint16_t flags0 : 4;
    uint16_t limit1 : 4;
    uint16_t flags1 : 4;
    uint16_t base2 : 8;
} __packed segdesc_t;

typedef struct {
    uint16_t limit0;
    uint16_t base0;
    uint16_t base1 : 8;
    uint16_t accel : 4;
    uint16_t flags0 : 4;
    uint16_t limit1 : 4;
    uint16_t flags1 : 4;
    uint16_t base2 : 8;
    uint32_t base3;
    uint32_t rsvd;
} __packed tssdesc_t;

#define TSS(base, limit, access, flags) ((tssdesc_t){ \
    .limit0 = (uint16_t)(limit),                      \
    .base0 = (uint16_t)(base),                        \
    .base1 = (uint8_t)((base) >> 16),                 \
    .accel = ((access)&0xF),                          \
    .flags0 = ((flags)&0xF),                          \
    .limit1 = ((limit) >> 16) & 0xF,                  \
    .flags1 = ((flags) >> 4) & 0xF,                   \
    .base2 = (uint8_t)((base) >> 24),                 \
    .base3 = (uint32_t)((base) >> 32),                \
    .rsvd = 0,                                        \
})

typedef struct {
    uint16_t limit;
    uintptr_t base;
} __packed descptr_t;

typedef struct {
    segdesc_t null;
    segdesc_t kcode64;
    segdesc_t kdata64;
    segdesc_t ucode64;
    segdesc_t udata64;
    tssdesc_t tss;
} __packed gdt_t;

#define SEG(base, limit, access, flags) ((segdesc_t){ \
    .limit0 = AND((limit), 0xFFFF),                   \
    .base0 = AND((base), 0xFFFF),                     \
    .base1 = AND(SHR((base), 16), 0xFF),              \
    .accel = AND((access), 0xF),                      \
    .flags0 = AND((flags), 0xF),                      \
    .limit1 = AND(SHR((limit), 16), 0xF),             \
    .flags1 = AND(SHR((flags), 4), 0xF),              \
    .base2 = AND(SHR((base), 24), 0xFF),              \
})

typedef struct {
    uint32_t base0 : 16;
    uint32_t sel : 16;
    uint32_t _ist : 8;
    uint32_t type : 4;
    uint32_t attr : 4;
    uint32_t base1 : 16;
    uint32_t base2;
    uint32_t rsvd;
} __packed idt_desc_t;

#define NIDT 256
typedef struct {
    idt_desc_t entry[NIDT];
} __packed idt_t;

#define TRAP_GATE(istrap, base, sel, dpl, ist) ((idt_desc_t){ \
    .base0 = (uint16_t)(base),                                \
    .sel = (uint16_t)(sel),                                   \
    ._ist = ((ist)&7),                                        \
    .type = ((istrap) ? 0xf : 0xe),                           \
    .attr = (BS(3) | SHL((dpl), 1)),                          \
    .base1 = (uint16_t)(SHR((base), 16)),                     \
    .base2 = (uint32_t)(SHR((base), 32)),                     \
    .rsvd = 0,                                                \
})

extern void tvinit(void);
extern void idt_init(void);

extern void gdt_init(void);
extern void loadgs_base(uintptr_t base);
extern void loadgdt64(descptr_t *, int cs, int gs, int ss);

extern void loadidt(descptr_t *);
extern void loadtr(uint64_t sel);

extern void tss_set(uintptr_t kstack, uint16_t desc);