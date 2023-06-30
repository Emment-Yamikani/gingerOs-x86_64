OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    . = 0x8000;
    .trampoline ALIGN (4K) : AT (ADDR (.trampoline))
    {
        .trampoline = .;
        *(.trampoline);
        .multiboot = .;
        *(.multiboot)
    }

    . = 0xC0100000;
    multi = .;
    ._linit ALIGN (4K) : AT (ADDR (._linit) - 0xC0000000)
    {
        ._linit = .;
        *(._linit)
    }
    multe = .;

    . = 0xFFFF800001000000;
	_kernel_start = .;
    .text ALIGN (4K) : AT (ADDR (.text) - 0xFFFF800000000000)
    {
        text = .;
        *(.text)
    }

    .__minit ALIGN (4K) : AT(ADDR(.__minit) - 0xFFFF800000000000)
    {
        __minit = .;
        *(.__minit*)
        __minit_end = .;
    }

    .__einit ALIGN (4K) : AT(ADDR(.__einit) - 0xFFFF800000000000)
    {
        __einit = .;
        *(.__einit*)
        __einit_end = .;
    }

    .__efini ALIGN (4K) : AT(ADDR(.__efini) - 0xFFFF800000000000)
    {
        __efini = .;
        *(.__efini*)
        __efini_end = .;
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

    .rodata ALIGN (4K) : AT (ADDR (.rodata) - 0xFFFF800000000000)
    {
        rodata = .;
        *(.rodata)
    }

    .data ALIGN (4K) : AT (ADDR (.data) - 0xFFFF800000000000)
    {
        rodata = .;
        *(.data)
    }

    .bss ALIGN (4K) : AT (ADDR (.bss) - 0xFFFF800000000000)
    {
        rodata = .;
        *(.bss)
    }

    _kernel_end = .;
}