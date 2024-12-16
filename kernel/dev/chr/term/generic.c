#include <bits/errno.h>
#include <dev/tty.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <sys/thread.h>

int tty_alloc(tty_t **ptp) {
    tty_t *tp = NULL;

    if (ptp == NULL)
        return -EINVAL;

    if (NULL == (tp = (tty_t *)kmalloc(sizeof *tp)))
        return -ENOMEM;

    memset(tp, 0, sizeof *tp);

    *ptp = tp;

    return 0;
}

void tty_free(tty_t *tp) {
    tty_assert(tp);

    kfree(tp);
}