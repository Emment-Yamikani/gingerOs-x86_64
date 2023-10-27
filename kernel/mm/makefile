pmm_dir=$(mm_dir)/pmm
vmm_dir=$(mm_dir)/vmm
liballoc_dir=$(mm_dir)/liballoc

include $(pmm_dir)/makefile
include $(vmm_dir)/makefile
include $(liballoc_dir)/makefile


mm_objs +=\
$(pmm_objs)\
$(vmm_objs)\
$(mm_dir)/page_cache.o