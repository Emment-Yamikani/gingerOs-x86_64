#include <api.h>

void fun(void) {
    loop()
        sys_thread_yield();
}

int main(int argc, char const*argv[]) {
    (void)argc, (void)argv;
    pid_t pid = sys_fork();

    meminfo_t mem;
    sys_getmemusage(&mem);

    if (pid == 0) {
        printf(
            "Free: %8d KiB\n"
            "Used: %8d KiB\n",
            mem.free,
            mem.used
        );
    }

    loop()
        sys_thread_yield();
    return 0xDEADBEEF;
}