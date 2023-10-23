#pragma once

#include <sys/system.h>

typedef struct {
    char *mod_name;
    void *mod_arg;
    int (*mod_init)();
    int (*mod_fini)();
} module_sym_t;

extern module_sym_t __builtin_mods[];
extern module_sym_t __builtin_mods_end[];

#define MODULE_INIT(name, a, i, f)               \
    module_sym_t __used_section(.__builtin_mods) \
        __mod_##name = {                         \
            .mod_name = #name,                   \
            .mod_arg = a,                        \
            .mod_init = i,                       \
            .mod_fini = f,                       \
    }

typedef struct {
    char    *sym_name;
    void    *sym_addr;
} symbl_t;

extern symbl_t ksym_table[];
extern symbl_t ksym_table_end[];

#define EXPORT_SYMBOL(name)                                                   \
    symbl_t __attribute__((used, section(".ksym_table"))) __exported_##name = { \
        .sym_name = #name,                                                    \
        .sym_addr = &name,                                                    \
    };

int modules_init();