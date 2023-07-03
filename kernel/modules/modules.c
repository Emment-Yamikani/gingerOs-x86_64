#include <modules/module.h>
#include <sys/thread.h>
#include <mm/kalloc.h>
#include <lib/printk.h>
#include <bits/errno.h>


int modules_init(void) {
    int err = 0;
    int retval = 0;
    size_t up = 0, valid = 0;
    extern char __minit[];
    extern char __mfini[];
    int (**builtin_mods)() = NULL;
    size_t nr = __mfini - __minit;

    builtin_mods = (int (**)())__minit;
    for (size_t i = 0; i < nr; ++i) {
        if (!(builtin_mods)[i])
            continue;
        valid++;
        if ((err = ((builtin_mods)[i])()))
            return err;
        if (!retval)
            up++;
    }
 
    if (up != valid)
        printk("WARNING: some devices did not start successfully\n");

    return 0;
}