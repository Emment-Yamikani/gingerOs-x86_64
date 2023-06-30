/*
 * @Author: Banda Emment Yamikani 
 * @Date: 2021-12-19 17:11:35 
 * @Last Modified by: Banda Emment Yamikani
 * @Last Modified time: 2021-12-19 18:56:28
 */
#ifndef BIOS_H
#define BIOS_H

#include <sys/system.h>

//bios data area
#define BDA     0x400
//extended bios data area
#define EBDA    ((uintptr_t)*((uint16_t *)(VMA2HI(BDA + 0xe))) << 4)
//bios rom below 1M
#define BIOSROM 0xe0000

#endif //BIOS_H