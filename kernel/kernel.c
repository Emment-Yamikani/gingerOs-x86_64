#include <fs/fs.h>
#include <fs/file.h>
#include <mm/vmm.h>
#include <sys/thread.h>

__noreturn void kthread_main(void) {
    printk("Welcome to \e[0;011m'Ginger OS'\e[0m.\n");
    builtin_threads_begin(NULL);

    int fd = open("/ramfs/makefile", O_RDONLY);
    char b[getpagesize()];
    read(fd, b, sizeof b);
    close(fd);

    printk(b);

    memory_usage();
    loop() thread_join(0, NULL, NULL);
}

