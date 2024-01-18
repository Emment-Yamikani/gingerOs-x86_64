# Compiler
CC := x86_64-elf-gcc
LD := x86_64-elf-ld
AR := x86_64-elf-ar
AS := x86_64-elf-as

# Common flags
CFLAGS := -O2 -g -nostdinc -nostdlib -lgcc \
	-std=gnu2x -Wall -march=x86-64 -Werror \
	-Wextra -mcmodel=large -mno-red-zone -mno-mmx -msse
CPPFLAGS :=

# Linker flags
LDFLAGS := -nostdlib -static -m elf_x86_64 \
	-z max-page-size=0x1000 -T

# Kernel flags
KERNEL_FLAGS := $(CFLAGS) $(CPPFLAGS) -ffreestanding \
	-D__x86_64__ -Ikernel/include

# User flags
USER_FLAGS := $(CFLAGS) $(CPPFLAGS) -fPIC

# Directories
USR_DIR := usr
USR_LIB := $(USR_DIR)/usr/lib

# Kernel directories
KERNEL_DIR := kernel
KERNEL_SUBDIRS := $(shell find $(KERNEL_DIR) -type d)
ISO_DIR := iso
RAMFS_DIR := ramfs

# Kernel source files
KERNEL_SOURCES := $(shell find $(KERNEL_DIR) -type f \( -name '*.c' -o -name '*.asm' -o -name '*.S' \))
KERNEL_OBJS := $(patsubst $(KERNEL_DIR)/%.c, $(KERNEL_DIR)/%.o, $(patsubst $(KERNEL_DIR)/%.asm, $(KERNEL_DIR)/%.o, $(patsubst $(KERNEL_DIR)/%.S, $(KERNEL_DIR)/%.o, $(KERNEL_SOURCES))))

# User source files
USER_SOURCES := $(wildcard $(USR_LIB)/*.c $(USR_LIB)/*.asm)
USER_OBJS := $(patsubst $(USR_LIB)/%.c, $(USR_LIB)/%.o, $(patsubst $(USR_LIB)/%.asm, $(USR_LIB)/%.o, $(USER_SOURCES)))

# Kernel linked objects
LINKED_OBJS := $(KERNEL_OBJS) font.o

# Shared object file
LIBG_SO := $(USR_LIB)/libg.so

# Make rules
all: lime.elf module _iso_ run

# Kernel rules
$(KERNEL_DIR)/%.o: $(KERNEL_DIR)/%.c
	$(CC) $(KERNEL_FLAGS) -MD -c $< -o $@

$(KERNEL_DIR)/%.o: $(KERNEL_DIR)/%.asm
	nasm $< -f elf64 -o $@

$(KERNEL_DIR)/%.o: $(KERNEL_DIR)/%.S
	$(CC) $(KERNEL_FLAGS) -MD -c $< -o $@

lime.elf: $(ISO_DIR)/boot/lime.elf

$(ISO_DIR)/boot/lime.elf: $(KERNEL_DIR)/kernel.ld $(LINKED_OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

# Shared library rules
$(USR_LIB)/%.o: $(USR_LIB)/%.c
	$(CC) $(USER_FLAGS) -MD -c $< -o $@

$(USR_LIB)/%.o: $(USR_LIB)/%.asm
	nasm $< -f elf64 -o $@

$(LIBG_SO): $(USER_OBJS)
	$(LD) $(LDFLAGS) --shared $^ -o $@

# Additional rule
font.o: font.tf
	$(LD) -r -b binary -o $@ $^

# Common rules
module:
	./mkdisk -o $(ISO_DIR)/modules/ramfs -d $(RAMFS_DIR)

_iso_:
	grub-mkrescue -o ginger.iso $(ISO_DIR)

run:
	qemu-system-x86_64 -smp 4 -m size=2G -cdrom ginger.iso -no-reboot -no-shutdown -vga std -chardev stdio,id=char0,logfile=serial.log,signal=off -serial chardev:char0

debug:
	objdump -d $(ISO_DIR)/boot/lime.elf -M intel > lime.asm

clean_debug:
	rm lime.asm

passwd:
	./crypt

clean:
	rm -rf $(KERNEL_OBJS) $(KERNEL_OBJS:.o=.d) $(LINKED_OBJS) $(LINKED_OBJS:.o=.d) $(USR_LIB)/*.o $(USR_LIB)/*.d $(LIBG_SO) ginger.iso $(ISO_DIR)/modules/initrd $(ISO_DIR)/boot/lime.elf $(ISO_DIR)/modules/* serial.log
