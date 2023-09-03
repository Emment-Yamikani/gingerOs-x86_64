#include <modules/module.h>
#include <sys/thread.h>
#include <mm/kalloc.h>
#include <lib/printk.h>
#include <bits/errno.h>

int modules_init(void) {
    int err = 0;
    int retval = 0;
    size_t up = 0, valid = 0;
    module_sym_t *mod = __builtin_mods;
    size_t nr = __builtin_mods_end - __builtin_mods;

    for (size_t i = 0; i < nr; ++i, mod++) {
        if (!mod->mod_init)
            continue;
        valid++;
        if ((err = (mod->mod_init)(mod->mod_arg)))
            return err;
        if (!retval)
            up++;
    }
 
    if (up != valid)
        printk("WARNING: some devices did not start successfully\n");

    return 0;
}