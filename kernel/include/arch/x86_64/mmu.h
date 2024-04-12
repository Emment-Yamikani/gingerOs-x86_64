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
    u32 rsvd0;
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
    u32 rsvd3 : 16;
    u32 iomap : 16;
} __packed tss_t;

typedef struct {
    u16 limit0;
    u16 base0;
    u16 base1 : 8;
    u16 accel : 4;
    u16 flags0 : 4;
    u16 limit1 : 4;
    u16 flags1 : 4;
    u16 base2 : 8;
} __packed segdesc_t;

typedef struct {
    u16 limit0;
    u16 base0;
    u16 base1 : 8;
    u16 accel : 4;
    u16 flags0 : 4;
    u16 limit1 : 4;
    u16 flags1 : 4;
    u16 base2 : 8;
    u32 base3;
    u32 rsvd;
} __packed tssdesc_t;

#define TSS(base, limit, access, flags) ((tssdesc_t){ \
    .limit0 = (u16)(limit),                           \
    .base0 = (u16)(base),                             \
    .base1 = (u8)((base) >> 16),                      \
    .accel = ((access) & 0xF),                        \
    .flags0 = ((flags) & 0xF),                        \
    .limit1 = ((limit) >> 16) & 0xF,                  \
    .flags1 = ((flags) >> 4) & 0xF,                   \
    .base2 = (u8)((base) >> 24),                      \
    .base3 = (u32)((base) >> 32),                     \
    .rsvd = 0,                                        \
})

typedef struct {
    u16 limit;
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
    u32 base0 : 16;
    u32 sel : 16;
    u32 _ist : 8;
    u32 type : 4;
    u32 attr : 4;
    u32 base1 : 16;
    u32 base2;
    u32 rsvd;
} __packed idt_desc_t;

#define NIDT 256
typedef struct {
    idt_desc_t entry[NIDT];
} __packed idt_t;

#define TRAP_GATE(istrap, base, sel, dpl, ist) ((idt_desc_t){ \
    .base0 = (u16)(base),                                \
    .sel = (u16)(sel),                                   \
    ._ist = ((ist)&7),                                        \
    .type = ((istrap) ? 0xf : 0xe),                           \
    .attr = (BS(3) | SHL((dpl), 1)),                          \
    .base1 = (u16)(SHR((base), 16)),                     \
    .base2 = (u32)(SHR((base), 32)),                     \
    .rsvd = 0,                                                \
})

extern void tvinit(void);
extern void idt_init(void);

extern void gdt_init(void);
extern void loadgs_base(uintptr_t base);
extern void loadgdt64(descptr_t *, int cs, int gs, int ss);

extern void loadidt(descptr_t *);
extern void loadtr(u64 sel);

extern void tss_set(uintptr_t kstack, u16 desc);