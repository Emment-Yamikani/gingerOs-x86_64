x86_64_dir=$(arch_dir)/x86_64
chipset_dir=$(arch_dir)/chipset
firmware_dir=$(arch_dir)/firmware

include $(chipset_dir)/makefile
include $(firmware_dir)/makefile
include $(x86_64_dir)/makefile

arch_objs+=\
$(arch_dir)/paging.o\
$(arch_dir)/signal.o\
$(arch_dir)/thread.o\
$(chipset_objs)\
$(firmware_objs)\
$(x86_64_objs)