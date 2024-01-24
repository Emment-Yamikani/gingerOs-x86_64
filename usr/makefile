cc=x86_64-elf-gcc
ld=x86_64-elf-ld
ar=x86_64-elf-ar
as=x86_64-elf-as

usr_flags:= -nostartfiles -ffreestanding -I$(usr_dir)/include -nostdlib -static \
-z max-page-size=0x1000 -T

app_dir=$(usr_dir)/apps

#directory having contents of our ramfs
ramfs_dir=ramfs

usrlibdir:=$(usr_dir)/lib

usrobjs:=\
$(usrlibdir)/_syscall.o\
$(usrlibdir)/ctype.o\
$(usrlibdir)/printf.o\
$(usrlibdir)/start.o\
$(usrlibdir)/string.o

$(usr_dir)/libc.a: $(usrobjs)
	$(ar) rcs $@ $(usrobjs)

usrlibc=$(usr_dir)/libc.a

linker_script=$(usr_dir)/linker.ld

%:  $(usrlibc) $(app_dir)/%.c
	$(cc) $^ $(usr_flags) $(linker_script) -o $(ramfs_dir)/$@

clean_usr:
	rm $(usrobjs) $(usrobjs:.o=.d) $(usrlibc)