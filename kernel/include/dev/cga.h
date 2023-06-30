#pragma once

#include <lib/stddef.h>
#include <lib/stdint.h>

extern int use_cga;

int cga_init(void);
void cga_putc(const int);
size_t cga_puts(const char *s);