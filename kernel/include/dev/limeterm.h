#pragma once

#include <lib/stddef.h>

#define _SC_UP      0xE2
#define _SC_LEFT    0xE4
#define _SC_RIGHT   0xE5
#define _SC_DOWN    0xE3

int limeterm_init(void);
void limeterm_clrscrn(void);
void limeterm_scroll(void);
void limeterm_putc(int c);
size_t limeterm_puts(const char *s);
void limeterm_sputc(int c);
void limeterm_clrscrn(void);

extern volatile int use_limeterm_cons;