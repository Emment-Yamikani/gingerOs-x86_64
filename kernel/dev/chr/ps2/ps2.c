#include <bits/errno.h>
#include <dev/ps2.h>
#include <lib/string.h>
#include <mm/kalloc.h>  
#include <sys/thread.h>
#include <modules/module.h>

extern int ps2kbd_init(void);
extern int ps2mouse_init(void);

static int ps2_init(void) {
    int err = 0;

    // TODO: initialize the ps2 controller.
    
    if ((err = ps2kbd_init()))
        return err;

    if ((err = ps2mouse_init()))
        return err;

    return 0;
} MODULE_INIT(ps2_controler, ps2_init, NULL, NULL);