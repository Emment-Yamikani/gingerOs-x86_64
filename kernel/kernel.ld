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
    }

    . = KERNEL_LMA + KERNEL_VMA;
    _kernel_start = .;
    .text ALIGN (4K) : AT (ADDR (.text) - KERNEL_VMA) {
        *(.text)
        *(.rodata)
    }

    .__builtin_thread_entry ALIGN (4K) : AT(ADDR(.__builtin_thread_entry) - 0xFFFF800000000000)
    {
        __builtin_thread_entry = .;
        *(.__builtin_thread_entry*)
        __builtin_thread_entry_end = .;
    }

    .__builtin_thread_arg ALIGN (4K) : AT(ADDR(.__builtin_thread_arg) - 0xFFFF800000000000)
    {
        __builtin_thread_arg = .;
        *(.__builtin_thread_arg*)
        __builtin_thread_arg_end = .;
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

    _kernel_end = .;
}