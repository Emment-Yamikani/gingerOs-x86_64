#include <fs/fs.h>
#include <fs/file.h>
#include <mm/vmm.h>
#include <sys/thread.h>
#include <mm/mmap.h>
#include <sys/proc.h>
#include <mm/mm_zone.h>

__noreturn void kthread_main(void) {
    printk("\n\t\t\tWelcome to \'\e[025453;011mGinger OS\e[0m\'.\n\n");
    builtin_threads_begin(NULL);

    printk( proc_init("/ramfs/test") == 0 ?
        "Successfully load \'init! :(\'\n" :
        "Failed to load \'init! :(\'\n"
    );

    loop() thread_join(0, NULL, NULL);
}

