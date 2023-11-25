#include <fs/fs.h>
#include <fs/file.h>
#include <mm/vmm.h>
#include <sys/thread.h>

__noreturn void kthread_main(void) {
    printk("Welcome to \e[025453;011m'Ginger OS'\e[0m.\n");
    builtin_threads_begin(NULL);

    memory_usage();
    loop() thread_join(0, NULL, NULL);
}

