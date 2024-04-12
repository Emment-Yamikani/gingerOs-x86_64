#pragma once

#include <arch/x86_64/context.h>
#include <arch/x86_64/paging.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <lib/types.h>

void arch_dumptable(pte_t *table);

/**
 * @brief dump the trap frame.
 * 
 * @param tf the trapframe to be dumped to screen.
 * @param halt loops forever if non-zero and returns otherwise.
 */
extern void dump_tf(tf_t *tf, int halt);

/**
 * @brief default pagefault handler for gingerOs.
 * 
 * @param vmr is the virtual memory region
 *  where the fault occured.
 * 
 * @param fault holds data that describes the fault.
 *
 * @return 0 on success and otherwise on error. 
 */
int default_pgf_handler(vmr_t *vmr, vm_fault_t *fault);

/**
 * @brief unmap the entire address space of current;y active PDBR.
 * page directory base register (PDBR) is a physical address placed
 * in a system register (CR3 on intel CPUs).
 */
extern void arch_unmap_full(void);

/**
 * @brief unmounts a virtual adress.
 * work in tandem with arch_mount() described below.
 * 
 * @param vaddr vitual adress to unmount.
 */
extern void arch_unmount(uintptr_t vaddr);

/**
 * @brief allocate a page directory (to be used as PDBR).
 * 
 * @param ref a pointer to a location large enough to store
 * an address.
 * 
 * @return 0 on sucess and non-zero otherwise. 
 */
extern int arch_getpgdir(uintptr_t *ref);

/**
 * @brief frees the page directory that was allocated with
 * arch_getpgdir();
 * 
 * @param pgidr the physical address of the page directory to be freed.
 */
extern void arch_putpgdir(uintptr_t pgidr);

/**
 * @brief unmaps the entire virtual address space
 * pointed to by pgdir.
 * 
 * @param pgdir (see above for a description of pgdir).
 */
extern void arch_fullvm_unmap(uintptr_t pgdir);

/**
 * @brief 
 * 
 * @param paddr 
 * @param pvp 
 * @return int 
 */
extern int arch_mount(uintptr_t paddr, void **pvp);

/**
 * @brief 
 * 
 * @param trapframe 
 */
extern void arch_do_page_fault(tf_t *trapframe);

/**
 * @brief 
 * 
 * @param vaddr 
 * @param sz 
 */
extern void arch_unmap_n(uintptr_t vaddr, usize sz);

/**
 * @brief
 *
 */
int arch_mprotect(uintptr_t vaddr, usize sz, int flags);

/**
 * @brief 
 * 
 * @param vaddr 
 * @param sz 
 */
extern void arch_pagefree(uintptr_t vaddr, usize sz);

/**
 * 
*/
extern int arch_lazycpy(uintptr_t dst, uintptr_t src);

/**
 * @brief 
 * 
 * @param sz 
 * @param addr 
 * @return int 
 */
extern int arch_pagealloc(usize sz, uintptr_t *addr);

/**
 * 
*/
extern void arch_unmap(int i4, int i3, int i2, int i1);

/**
 * @brief 
 * 
 * @param pdbr 
 * @param old 
 */
extern void arch_swtchvm(uintptr_t pdbr, uintptr_t *old);

/**
 * @brief 
 * 
 * @param addr 
 * @param pte 
 * @return int 
 */
extern int arch_getmapping(uintptr_t addr, pte_t **pte);

/**
 * @brief 
 * 
 * @param vaddr 
 * @param sz 
 * @param flags 
 * @return int 
 */
extern int arch_map_n(uintptr_t vaddr, usize sz, int flags);

/**
 * @brief 
 * 
 * @param paddr 
 * @param vaddr 
 * @param size 
 * @return int 
 */
extern int arch_memcpyvp(uintptr_t paddr, uintptr_t vaddr, usize size);

/**
 * @brief 
 * 
 * @param vaddr 
 * @param paddr 
 * @param size 
 * @return int 
 */
extern int arch_memcpypv(uintptr_t vaddr, uintptr_t paddr, usize size);

/**
 * @brief 
 * 
 * @param vaddr 
 * @param paddr 
 * @param sz 
 * @param flags 
 * @return int 
 */
extern int arch_map_i(uintptr_t vaddr, uintptr_t paddr, usize sz, int flags);

/**
 * @brief 
 * 
 * @param pdst 
 * @param psrc 
 * @param size 
 * @return int 
 */
extern int arch_memcpypp(uintptr_t pdst, uintptr_t psrc, usize size);

/**
 * @brief 
 * 
 * @param frame 
 * @param i4 
 * @param i3 
 * @param i2 
 * @param i1 
 * @param flags 
 * @return int 
 */
extern int arch_map(uintptr_t frame, int i4, int i3, int i2, int i1, int flags);