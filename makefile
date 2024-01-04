cc=x86_64-elf-gcc
ld=x86_64-elf-ld
ar=x86_64-elf-ar
as=x86_64-elf-as

_arch_=__x86_64__

cflags += -O2 -g -nostdinc -nostdlib \
 	-lgcc -std=gnu2x -Wall -march=x86-64\
 	-Werror -Wextra -mcmodel=large \
 	-mno-red-zone -mno-mmx -msse

cppflags +=

ldflags += -nostdlib -static -m elf_x86_64 \
	-z max-page-size=0x1000 -T

kernel_flags:=$(cflags) $(cppflags) -ffreestanding -D$(_arch_) -Ikernel/include

#currently only supports the intel x86-32bit 386 or higher
_ARCH_=i386

kernel_dir=kernel

include $(kernel_dir)/makefile

#directory having iso-image recipe(contents)
iso_dir=iso

#directory having contents of our ramfs
ramfs_dir=ramfs

linked_objs:=\
$(kernel_objs)\
font.o

.PHONY: all clean

.SUFFIXES: .o .asm .s .c

.c.o:
	$(cc) $(kernel_flags) -MD -c $< -o $@

.s.o:
	$(cc) $(kernel_flags) -MD -c $< -o $@

.tf.o:
	$(ld) -r -b binary -o $@ $^

.asm.o:
	nasm $< -f elf64 -o $@

all: lime.elf module _iso_ debug run

font.o: font.tf
	$(ld) -r -b binary -o $@ $^

lime.elf: $(iso_dir)/boot/lime.elf

#$(iso_dir)/boot/lime.elf: $(kernel_dir)/linker.ld $(linked_objs)
#	$(ld) $(ldflags) $^ -o $@ $(kernel_flags)

$(iso_dir)/boot/lime.elf: $(kernel_dir)/kernel.ld $(linked_objs)
	$(ld) $(ldflags) $^ -o $@

run:
	qemu-system-x86_64	\
	-smp 1	 			\
	-m size=2G			\
	-cdrom	ginger.iso	\
	-no-reboot			\
	-no-shutdown		\
	-vga std			\
	-chardev stdio,\
	id=char0,\
	logfile=serial.log,\
	signal=off 			\
    -serial chardev:char0

_iso_:
	grub-mkrescue -o ginger.iso $(iso_dir)

module:
	./mkdisk -o $(iso_dir)/modules/ramfs -d $(ramfs_dir)

debug:
	objdump -d iso/boot/lime.elf -M intel > lime.asm

clean_debug:
	rm lime.asm

passwd:
	./crypt

clean:
	rm $(linked_objs) $(linked_objs:.o=.d) ginger.iso lime.asm $(iso_dir)/modules/initrd $(iso_dir)/boot/lime.elf $(iso_dir)/modules/* serial.log

usr_dir=usr

include $(usr_dir)/makefile