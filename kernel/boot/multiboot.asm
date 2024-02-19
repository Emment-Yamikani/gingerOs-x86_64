[bits 32]

MODS_ALIGN  EQU (1 << 0)
MEMINFO     EQU (1 << 1)
GFXMODE     EQU (1 << 2)
MAGIC       EQU (0x1BADB002);(0xE85250D6);
FLAGS       EQU (MODS_ALIGN | MEMINFO | GFXMODE)
CHECKSUM    EQU -(MAGIC + FLAGS)

section .multiboot
align 4

mboot:
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; section header info(Not of concern)

    dd 0x00
    dd 0x00
    dd 0x00
    dd 0x00
    dd 0x00

; gfx parameters
    LFB_GFX     EQU (0)
    EGA_TXT     EQU (1)
    GFX_WIDTH   EQU (1440)
    GFX_HEIGHT  EQU (800)
    GFX_DEPTH   EQU (32)

    dd LFB_GFX
    dd GFX_WIDTH
    dd GFX_HEIGHT
    dd GFX_DEPTH
;