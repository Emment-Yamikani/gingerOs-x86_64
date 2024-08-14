#pragma once

extern void    swapi8(char  *dst, char *src);
extern void    swapi16(short *dst, short *src);
extern void    swapi32(int  *dst, int  *src);
extern void    swapi64(long *dst, long *src);
extern void    swapptr(void **p0, void **p1);

extern void    swapu8(unsigned char  *dst, unsigned char *src);
extern void    swapu16(unsigned short *dst, unsigned short *src);
extern void    swapu32(unsigned int  *dst, unsigned int  *src);
extern void    swapu64(unsigned long *dst, unsigned long *src);