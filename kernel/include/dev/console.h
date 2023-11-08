#pragma once

extern int earlycons_use_gfx;
extern int use_earlycons;

void earlycons_usefb(void);
int earlycons_putc(int c);

int console_putc(int c);