#pragma once

#include <sys/system.h>

#define MODULE_INIT(name, i, f)                            \
    __section(".__minit") void *__CAT(__minit_, name) = i; \
    __section(".__mfini") void *__CAT(__mfini_, name) = f;


#define EXPORT_SYMBOL(name, func) \
    __section(".kernel_symbols")

int modules_init();