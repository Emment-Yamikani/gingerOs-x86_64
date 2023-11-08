cga_dir=$(chrdev_dir)/cga
console_dir=$(chrdev_dir)/console
fb_dir=$(chrdev_dir)/fb
uart_dir=$(chrdev_dir)/uart
timers_dir=$(chrdev_dir)/timers
pseudodev_dir=$(chrdev_dir)/pseudodev
limeterm_dir=$(chrdev_dir)/limeterm

include $(cga_dir)/makefile
include $(console_dir)/makefile
include $(timers_dir)/makefile
include $(limeterm_dir)/makefile

chrdev_objs:=\
$(limeterm_objs)\
$(fb_dir)/fbdev.o\
$(uart_dir)/uart.o\
$(pseudodev_dir)/full.o\
$(pseudodev_dir)/null.o\
$(pseudodev_dir)/random.o\
$(pseudodev_dir)/zero.o\
$(cga_objs)\
$(console_objs)\
$(timers_objs)