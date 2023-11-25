#include <fs/fs.h>
#include <fs/file.h>
#include <mm/vmm.h>
#include <sys/thread.h>
#include <fs/path.h>

__noreturn void kthread_main(void) {
    printk("Welcome to \e[0;011m'Ginger OS'\e[0m.\n");
    builtin_threads_begin(NULL);
    
    path_t *path = NULL;

    path_process("/bin/fin", NULL, 0, &path);

    printk("abspath: %s\n"
        "tokens: %p\n"
        "tokcnt: %ld\n"
        "lasktok: %s\n",
        path->absolute,
        path->tokenized,
        path->tokencount,
        path->lasttoken);

    memory_usage();
    loop() thread_join(0, NULL, NULL);
}

