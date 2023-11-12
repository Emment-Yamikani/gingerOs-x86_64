#include <fs/fs.h>
#include <fs/file.h>
#include <sys/thread.h>

__noreturn void kthread_main(void) {
    printk("Welcome to \e[0;011m'Ginger OS'\e[0m.\n");
    builtin_threads_begin(NULL);
    
    int fd = open("/ramfs/init.rc", O_RDONLY);
    char b[4092];
    read(fd, b, sizeof b);
    close(fd);

    printk("\n\e[0;077777045m%s\e[0m\n", b);

    loop() thread_join(0, NULL, NULL);
}

