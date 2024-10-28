# Compiler
CC := x86_64-elf-gcc
LD := x86_64-elf-ld
AR := x86_64-elf-ar
AS := x86_64-elf-as

# Common flags
CFLAGS := -O2 -g -ffreestanding -nostdinc -nostdlib -lgcc \
	-nostartfiles -std=gnu2x -Wall -march=x86-64 -Werror \
	-Wextra -mcmodel=large -mno-red-zone -mno-mmx -msse
CPPFLAGS :=

# Linker flags
LDFLAGS := -nostdlib -static -m elf_x86_64 \
	-z max-page-size=0x1000

#User linker flags
USER_LDFLAGS := -nostdlib -static -m elf_x86_64 \
	-z max-page-size=0x1000

# Kernel flags
KERNEL_FLAGS := $(CFLAGS) $(CPPFLAGS) -ffreestanding \
	-D__x86_64__ -Ikernel/include

#User flags
USER_FLAGS := $(CFLAGS) $(CPPFLAGS) -Iusr/include

# User lib flags
USER_LIB_FLAGS := $(CFLAGS) $(CPPFLAGS) -fPIC -Iusr/include

# Directories
USR_DIR := usr
APP_DIR := $(USR_DIR)/app
USR_LIB := $(USR_DIR)/lib

# Kernel directories
ISO_DIR := iso
RAMFS_DIR := ramfs
KERNEL_DIR := kernel
KERNEL_SUBDIRS := $(shell find $(KERNEL_DIR) -type d)

# Kernel source files
KERNEL_SOURCES := $(shell find $(KERNEL_DIR) -type f \( -name '*.c' -o -name '*.asm' -o -name '*.S' \))
KERNEL_OBJS := $(patsubst $(KERNEL_DIR)/%.c, $(KERNEL_DIR)/%.o, $(patsubst $(KERNEL_DIR)/%.asm, $(KERNEL_DIR)/%.o, $(patsubst $(KERNEL_DIR)/%.S, $(KERNEL_DIR)/%.o, $(KERNEL_SOURCES))))

USR_SOURCES := $(wildcard $(USR_DIR)/*.c $(USR_DIR)/*.asm)
USR_OBJS := $(patsubst $(USR_DIR)/%.c, $(USR_DIR)/%.o, $(patsubst $(USR_DIR)/%.asm, $(USR_DIR)/%.o, $(USR_SOURCES)))

# User library source files
USER_LIB_SOURCES := $(shell find $(USR_LIB) -type f \( -name '*.c' -o -name '*.asm' -o -name '*.S' \))
USER_LIB_OBJS := $(patsubst $(USR_LIB)/%.c, $(USR_LIB)/%.o, $(patsubst $(USR_LIB)/%.asm, $(USR_LIB)/%.o, $(USER_LIB_SOURCES)))

# Shared object file
LIBC_SO := $(USR_LIB)/libc.so

# Kernel linked objects
LINKED_OBJS := $(KERNEL_OBJS) font.o

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
	$(LD) $(LDFLAGS) -T $^ -o $@

# Additional rule
font.o: font.tf
	$(LD) -r -b binary -o $@ $^

# Common rules
module:
	./mkdisk -o $(ISO_DIR)/modules/ramfs -d $(RAMFS_DIR)

_iso_:
	grub-mkrescue -o ginger.iso $(ISO_DIR)

debug:
	objdump -d $(ISO_DIR)/boot/lime.elf -M intel > lime.asm

run:
	qemu-system-x86_64 -smp 2 \
	-m size=512M -cdrom ginger.iso \
	-no-reboot -no-shutdown -vga std \
	-chardev stdio,id=char0,logfile=serial.log,signal=off \
	-serial chardev:char0

######################################
#			 USER RULES			     #
###################################### 

# Shared library rules
$(USR_LIB)/%.o: $(USR_LIB)/%.c
	$(CC) $(USER_LIB_FLAGS) -MD -c $< -o $@

$(USR_LIB)/%.o: $(USR_LIB)/%.asm
	nasm $< -f elf64 -o $@

#$(LIBC_SO): $(USER_LIB_OBJS)
#	$(LD) $(USER_LDFLAGS) --shared $^ -o $@

$(LIBC_SO): $(USER_LIB_OBJS)
	$(AR) rcs usr/lib/libc.a $^

LIBC: $(LIBC_SO)

$(USR_DIR)/%.o: $(USR_DIR)/%.c
	$(CC) $(USER_FLAGS) -MD -c $< -o $@

$(USR_DIR)/%.o: $(USR_DIR)/%.asm
	nasm $< -f elf64 -o $@

$(APP_DIR)/%.o: $(APP_DIR)/%.c
	$(CC) $(USER_FLAGS) -MD -c $< -o $@

$(APP_DIR)/%.o: $(APP_DIR)/%.asm
	nasm $< -f elf64 -o $@

APP_OBJ := $(APP_DIR)/%.o

%: $(LIBC_SO) $(APP_OBJ) $(USR_OBJS)
	$(LD) $(USER_LDFLAGS) -T $(USR_DIR)/linker.ld $(APP_DIR)/$@.o $(USR_OBJS) $(USR_LIB)/libc.a -o $(RAMFS_DIR)/$@
	make

clean_debug:
	rm lime.asm

clean:
	rm -rf $(KERNEL_OBJS) $(KERNEL_OBJS:.o=.d) $(LINKED_OBJS) $(LINKED_OBJS:.o=.d) $(USER_LIB_OBJS) $(USER_LIB_OBJS:.o=.d) $(LIBC_SO) $(USR_OBJS) $(USR_LIB)/libc.a $(USR_OBJS:.o=.d) $(APP_DIR)/*.o $(APP_DIR)/*.d ginger.iso $(ISO_DIR)/modules/initrd $(ISO_DIR)/boot/lime.elf $(ISO_DIR)/modules/* serial.log

clean_usr:
	rm -rf $(USER_LIB_OBJS) $(USER_LIB_OBJS:.o=.d) $(LIBC_SO) $(USR_OBJS) $(USR_LIB)/libc.a $(USR_OBJS:.o=.d) $(APP_DIR)/*.o $(APP_DIR)/*.d