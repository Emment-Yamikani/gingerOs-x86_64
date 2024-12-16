#pragma once

#include <core/mutex.h>
#include <dev/dev.h>
#include <sync/spinlock.h>

#define NTTY    8

typedef struct tty_t {
    dev_t       *dev;
} tty_t;



#define tty_assert(__tp)  ({ assert(__tp, "No tty."); })

extern int tty_alloc(tty_t **ptp);
extern void tty_free(tty_t *tp);

extern tty_t *tty_current(void);