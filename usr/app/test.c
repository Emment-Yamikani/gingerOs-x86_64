#include <api.h>

void fun(void) {
    loop() sys_thread_yield();
}

int main(int argc, char const*argv[]) {
    (void)argc, (void)argv;
    
    int fd = sys_open("/ramfs/startup.conf", O_RDONLY, 0);
    pid_t pid = sys_fork();


    if (pid < 0)
        sys_exit(pid);
    else if (pid == 0) {
        int err = 0;
        char buf[1024] = {0};

        if (sys_fork() == 0)
            loop() sys_thread_yield();

        sys_thread_create(NULL, NULL, (thread_entry_t)fun, NULL);

        if ((err = sys_read(fd, buf, 1024)) < 0)
            panic("failed to read from file: %d\n", err);

        sys_lseek(fd, 0, SEEK_SET);

        printf( "File startup.conf:\n%s\n*EOF*\n", buf);

        meminfo_t info;
        sys_getmemusage(&info);

        printf("Used: %d KiB\n"
            "Free: %d KiB\n",
            info.used,
            info.free
        );
        sys_exit(0);
    }

    if (sys_getpid() == 1)
        loop() sys_thread_yield();
    return 0xDEADBEEF;
}