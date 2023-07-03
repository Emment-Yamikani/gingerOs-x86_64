chrdev_dir=$(dev_dir)/chrdev
blkdev_dir=$(dev_dir)/blkdev

include $(chrdev_dir)/makefile
include $(blkdev_dir)/makefile

dev_objs:=\
$(dev_dir)/dev.o\
$(blkdev_objs)\
$(chrdev_objs)