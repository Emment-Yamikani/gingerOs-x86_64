#include <fs/fs.h>
#include <fs/file.h>
#include <mm/vmm.h>
#include <sys/thread.h>
#include <mm/mmap.h>
#include <sys/proc.h>
#include <mm/mm_zone.h>
#include <mm/kalloc.h>

int load_init(const char *conf_fn);

__noreturn void kthread_main(void) {
    int     err     = 0;

    printk("\n\t\t\tWelcome to \'"
        "\e[025453;011mGinger "
        "OS\e[0m\'.\n\n"
    );

    builtin_threads_begin(NULL);

    if ((err = load_init("/ramfs/startup.conf"))) {
        printk("Failed to read or parse startup.conf"
            "\nexit_code: %d\n", err
        );
    }

    loop() thread_join(0, NULL, NULL);
}

static int conf_find(const char *buf, const char *str, char **ret) {
    char    *at     = NULL;
    char    *cpybuf = NULL;

    if (buf == NULL || str == NULL || ret == NULL)
        return -EINVAL;
    
    if (NULL == (cpybuf = strdup(buf)))
        return -ENOMEM;

    for (at = strtok(cpybuf, "\n\t"); (at = strtok(NULL, "\n\t")); ) {
        printk("value: %s\n", at);
    }

    return 0;
}

int load_init(const char *conf_fn) {
    int     err       = 0;
    int     conf_fd   = 0;
    size_t  conf_size = 0;
    char    *conf_buf = NULL;
    char    *init_desc= NULL;

    if (conf_fn == NULL)
        return -EINVAL;

    if ((err = open(conf_fn, O_RDONLY, 0)) < 0) {
        printk("Failed to open startup configuration file,"
            "failed with err_code: %d\n", err
        );
        goto error;
    }

    conf_size = lseek(conf_fd, 0, SEEK_END) + 1;
    lseek(conf_fd, 0, SEEK_SET);

    if (NULL == (conf_buf = kcalloc(1, conf_size))) {
        printk("failed to allocate buf for configuration file\n"
            "Not enough memory to satisfy request!\n"
        );
        goto error;
    }

    if ((ssize_t)(err = read(conf_fd, conf_buf, conf_size)) != (ssize_t)conf_size) {
        printk("Failed to read startup configuration file\n"
            "read() returned: %d\n", err
        );
    }

    printk("Parsing startup configuration file...\n");
    conf_find(conf_buf, "![init]", &init_desc);

    printk("Loading of bootstrap program %s.\n", 
        proc_init("/ramfs/test") == 0 ? 
        "Successful :)" : "Unsucessful :("
    );

    return 0;
error:
    return err;
}