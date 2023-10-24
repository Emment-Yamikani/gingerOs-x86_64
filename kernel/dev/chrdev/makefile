cga_dir=$(chrdev_dir)/cga
console_dir=$(chrdev_dir)/console
timers_dir=$(chrdev_dir)/timers
pseudodev_dir=$(chrdev_dir)/pseudodev

include $(cga_dir)/makefile
include $(console_dir)/makefile
include $(timers_dir)/makefile

chrdev_objs:=\
$(pseudodev_dir)/full.o\
$(pseudodev_dir)/null.o\
$(pseudodev_dir)/random.o\
$(pseudodev_dir)/zero.o\
$(cga_objs)\
$(console_objs)\
$(timers_objs)