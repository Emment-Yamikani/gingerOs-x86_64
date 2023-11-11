#include <fs/fs.h>
#include <fs/file.h>
#include <sys/thread.h>

__noreturn void kthread_main(void) {
    printk("Welcome to \e[0;011m'Ginger OS'\e[0m.\n");
    builtin_threads_begin(NULL);
    __unused int fd = 0, fd2 = 0;
    char buf[PGSZ];

    fd = open("/ramfs/init.rc", O_RDONLY, 0);
    fd2 = dup2(fd, 3);
    close(fd);

    read(fd2, buf, sizeof buf);
    close(fd2);
    
    printk("\e[0;02m%s\e[0m\n", buf);

    loop() thread_join(0, NULL, NULL);
}

