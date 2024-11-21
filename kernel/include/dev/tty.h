#pragma once

#include <dev/dev.h>

#define NTTY    8

typedef struct tty_t {
    dev_t       *dev;
} tty_t;