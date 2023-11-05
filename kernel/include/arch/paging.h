#pragma once

#include <lib/stdint.h>
#include <arch/x86_64/pml4.h>

// TLB flush
extern void tlb_flush(void);

/**
 * 
*/
extern int get_mapping(pagemap_t *map, uintptr_t v, pte_t **ppte);

/**
 * 
*/
extern int mount_page();

/**
 * 
*/
extern int unmount_page();

/**
 * 
*/
extern int unmappdbr(pagemap_t *map);

/**
 * 
*/
extern void invlpg_n(uintptr_t v, size_t sz);

/**
 * 
*/
extern int mapt(pagemap_t *map, size_t t);

/**
 * 
*/
extern int unmapt(pagemap_t *map, size_t t);

/**
 * 
*/
extern int unmap(pagemap_t *map, uintptr_t v);

/**
 * 
*/
extern int unmap_n(pagemap_t *map, uintptr_t v, size_t sz);

/**
 * 
*/
extern int mapv_n(pagemap_t *map, uintptr_t v, size_t sz, int flags);

/**
 * 
*/
extern int mappv(pagemap_t *map, uintptr_t p, uintptr_t v, size_t sz, int flags);

/**
 * 
*/
extern int swtchkvm(void);

/**
 * 
*/
extern int swtchvm(pagemap_t *map);

/**
 * 
*/
extern uintptr_t getpdbr(void);

/**
 * 
*/
extern int kvmcpy(pagemap_t *dst);

/**
 * 
*/
extern int lazycpy(pagemap_t *dst, pagemap_t *src);

/**
 * 
*/
extern int memcpypp(uintptr_t pdst, uintptr_t psrc, size_t sz);

/**
 * 
*/
extern int memcpypv(uintptr_t vdst, uintptr_t psrc, size_t sz);

/**
 * 
*/
extern int memcpyvp(uintptr_t pdst, uintptr_t vsrc, size_t sz);

/**
 * 
*/
extern int memcpyvv(uintptr_t vdst, uintptr_t vsrc, size_t sz);