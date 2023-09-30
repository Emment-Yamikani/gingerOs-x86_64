#pragma once

#include <lib/stddef.h>
#include <lib/stdint.h>

extern int use_cga;

#define CGA_BLACK           0
#define CGA_BLUE            1
#define CGA_GREEN           2
#define CGA_CYAN            3
#define CGA_RED             4
#define CGA_MAGENTA         5
#define CGA_BROWN           6
#define CGA_LIGHT_GREY      7
#define CGA_DARK_GREY       8
#define CGA_LIGHT_BLUE      9
#define CGA_LIGHT_GREEN     10
#define CGA_LIGHT_CYAN      11
#define CGA_LIGHT_RED       12
#define CGA_LIGHT_MAGENTA   13
#define CGA_YELLOW          14
#define CGA_WHITE           15

int cga_init(void);
void cga_putc(const int);
size_t cga_puts(const char *s);