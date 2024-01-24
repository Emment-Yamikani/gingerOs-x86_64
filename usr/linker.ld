ENTRY(start)

SECTIONS {
	. = 0x4000000;
	phys = .;

	.text BLOCK(4K) : ALIGN(4K) {
		code = .;
		KEEP (*(.text))
	}

	.rodata BLOCK(4K) : ALIGN(4K) {
		KEEP (*(.rodata))
	}

	.data BLOCK(4K) : ALIGN(4K) {
		KEEP (*(.data))
	}

	.bss BLOCK(4K) : ALIGN(4K) {
		*(COMMON)
		KEEP (*(.bss))
	}

	.eh_frame BLOCK(4K) : ALIGN(4K) {
		KEEP (*(.eh_frame))
	}

	.init_array : {
		PROVIDE_HIDDEN (__init_array_start = .);
		*(.init_array);
		PROVIDE_HIDDEN (__init_array_end = .);
	}

	end = .;

	/DISCARD/ : {
		*(.comment)
		*(.note.gnu.build-id)
	}
}
