ENTRY(start32)
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

TRAMPOLINE = 0x8000;
KERNEL_LMA = 0x1000000;
KERNEL_VMA = 0xFFFF800000000000;


SECTIONS {
    . = TRAMPOLINE;
    .trampoline ALIGN (4K) : AT (ADDR (.trampoline)) {
        *(.trampoline)
        *(.multiboot)
        *(.init32)
    }

    . = KERNEL_LMA + KERNEL_VMA;
    kstart = .;
    .text ALIGN (4K) : AT (ADDR (.text) - KERNEL_VMA) {
        *(.text)
    }

    .ksym_table ALIGN (4K) : AT(ADDR(.ksym_table) - KERNEL_VMA) {
        ksym_table = .;
        *(.ksym_table*)
        ksym_table_end = .;
    }

    .__builtin_mods ALIGN (4K) : AT(ADDR(.__builtin_mods) - KERNEL_VMA) {
        __builtin_mods = .;
        *(.__builtin_mods*)
        __builtin_mods_end = .;
    }

    .__builtin_thrds ALIGN (4K) : AT(ADDR(.__builtin_thrds) - KERNEL_VMA) {
        __builtin_thrds = .;
        *(.__builtin_thrds*)
        __builtin_thrds_end = .;
    }

    .rodata ALIGN (4K) : AT (ADDR (.rodata) - KERNEL_VMA) {
        *(.rodata)
    }

    .data ALIGN (4K) : AT (ADDR (.data) - KERNEL_VMA) {
        *(.data)
    }

    .bss ALIGN (4K) : AT (ADDR (.bss) - KERNEL_VMA) {
        *(.bss)
        *(COMMON)
    }

    /DISCARD/ :
    {
      *(.comment)
      /* Exception handling section - we do not need it*/
      *(.eh_frame)
    }

    kend = .;
}