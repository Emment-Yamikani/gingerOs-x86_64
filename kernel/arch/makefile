x86_64_dir=$(arch_dir)/x86_64
firmware_dir=$(arch_dir)/firmware
chipset_dir=$(arch_dir)/chipset

include $(firmware_dir)/makefile
include $(x86_64_dir)/makefile
include $(chipset_dir)/makefile

arch_objs:=\
$(x86_64_objs)\
$(firmware_objs)\
$(chipset_objs)