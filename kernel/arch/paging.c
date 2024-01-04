#include <arch/x86_64/cpu.h>
#include <arch/paging.h>
#include <bits/errno.h>
#include <fs/icache.h>
#include <mm/mmap.h>
#include <mm/vmm.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <arch/signal.h>
#include <sys/syscall.h>
#include <mm/mm_zone.h>
#include <mm/pmm.h>

void arch_dumptable(pte_t *table) {
    x86_64_dumptable(table);
}

void arch_swtchvm(uintptr_t pdbr, uintptr_t *old) {
#if defined (__x86_64__)
    x86_64_swtchvm(pdbr, old);
#endif
}

int arch_map(uintptr_t frame, int i4, int i3, int i2, int i1, int flags) {
#if defined (__x86_64__)
    return x86_64_map(frame, i4, i3, i2, i1, flags);
#endif
}

void arch_unmap(int i4, int i3, int i2, int i1) {
#if defined (__x86_64__)
    return x86_64_unmap(i4, i3, i2, i1);
#endif
}

void arch_unmap_n(uintptr_t v, size_t sz) {
#if defined (__x86_64__)
    return x86_64_unmap_n(v, sz);
#endif
}

int arch_map_i(uintptr_t v, uintptr_t p, size_t sz, int flags) {
#if defined (__x86_64__)
    return x86_64_map_i(v, p, sz, flags);
#endif
}

int arch_map_n(uintptr_t v, size_t sz, int flags) {
#if defined (__x86_64__)
    return x86_64_map_n(v, sz, flags);
#endif
}

int arch_mount(uintptr_t p, void **pvp) {
#if defined (__x86_64__)
    return x86_64_mount(p, pvp);
#endif
}

void arch_unmount(uintptr_t v) {
#if defined (__x86_64__)
    return x86_64_unmount(v);
#endif
}

void arch_unmap_full(void) {
#if defined (__x86_64__)
    return x86_64_unmap_full();
#endif
}

void arch_fullvm_unmap(uintptr_t pml4) {
#if defined (__x86_64__)
    return x86_64_fullvm_unmap(pml4);
#endif
}

int arch_lazycpy(uintptr_t dst, uintptr_t src) {
#if defined (__x86_64__)
    return x86_64_lazycpy(dst, src);
#endif
}

int arch_memcpypp(uintptr_t pdst, uintptr_t psrc, size_t size) {
#if defined (__x86_64__)
    return x86_64_memcpypp(pdst, psrc, size);
#endif
}

int arch_memcpyvp(uintptr_t p, uintptr_t v, size_t size) {
#if defined (__x86_64__)
    return x86_64_memcpyvp(p, v, size);
#endif
}

int arch_memcpypv(uintptr_t v, uintptr_t p, size_t size) {
#if defined (__x86_64__)
    return x86_64_memcpypv(v, p, size);
#endif
}

int arch_getmapping(uintptr_t addr, pte_t **pte) {
#if defined (__x86_64__)
    return x86_64_getmapping(addr, pte);
#endif
}

int arch_getpgdir(uintptr_t *ref) {
#if defined (__x86_64__)
    return x86_64_pml4alloc(ref);
#endif
}

void arch_putpgdir(uintptr_t pgdir) {
#if defined (__x86_64__)
    x86_64_pml4free(pgdir);
#endif
}

int arch_pagealloc(size_t sz, uintptr_t *addr) {
    int err = 0;
    uintptr_t v = 0;

    if (addr == NULL)
        return -EINVAL;

    if ((v = vmman.alloc(sz)) == 0)
        return -ENOMEM;
    

    if ((err = arch_map_n(v, sz, PTE_KRW))) {
        debugloc();
        vmman.free(v);
        return err;
    }

    *addr = v;
    return 0;
}

void arch_pagefree(uintptr_t v, size_t sz) {
    arch_unmap_n(v, sz);
    vmman.free(v);
}

void arch_tlbshootdown(uintptr_t pdbr, uintptr_t vaddr) {
#if defined (__x86_64__)
    send_tlb_shootdown(pdbr, vaddr);
#endif
}

void arch_do_page_fault(tf_t *trapframe) {
    int         err             = 0;
    vm_fault_t  fault           = {0};
    mmap_t      *mmap           = NULL;
    vmr_t       *vmr            = NULL;

    pushcli(); // temporarily get rid of interrupts.

    fault.addr         = rdcr2();
    fault.err_code     = trapframe->err_code;
#if defined (__x86_64__) // get x86_64 specific
    fault.user         = x86_64_tf_isuser(trapframe);
#endif
    err                = arch_getmapping(fault.addr, &fault.COW);

    if (current) {
#if defined(__x86_64__)
        /** if rip is 0xffffffffffffffff user either
         *  thread explicitly returned from entrypoint function,
         * or current is returning from a signal handler.
         */
        if (trapframe->rip == (-1ull)) {
#endif
            /* return from a signal handler.*/
            if (current_ishandling())
                arch_signal_return();
            else if (current_ismain()) {
#if defined(__x86_64__) /*exit from main thread*/
                exit(trapframe->rax);
#elif defined(__aarch64__)
                exit(trapframe->rax);
#endif
            }
            else { /*thread exit*/
#if defined(__x86_64__)
                thread_exit(trapframe->rax);
#elif defined(__aarch64__)
                thread_exit(trapframe->rax);
#endif
            }
            goto done;
        }

        current_lock();
        mmap = current->t_mmap;
        /// TODO: increase the reference count on mmap;
        current_unlock();
    }

    if (mmap == NULL)
        goto kernel_fault;

    /*Access to kernel address?*/
    if (iskernel_addr(fault.addr))
        goto kernel_fault;

    mmap_lock(mmap);
    // is this address in the virtual address space?
    if (NULL == (vmr = mmap_find(mmap, fault.addr))) {
        mmap_unlock(mmap);
        goto send_SIGSEGV;
    }

    // vmr has specific page fault handler?
    if (vmr->vmops == NULL || vmr->vmops->fault == NULL) {
        // call default page fault handler.
        if ((err = default_pgf_handler(vmr, &fault)) == -EFAULT) {
            mmap_unlock(mmap);
            goto send_SIGBUS;
        } else if (err) {
            mmap_unlock(mmap);
            goto send_SIGSEGV;
        }
    
    // call vmr specific page fault handler.
    } else if ((err = vmr->vmops->fault(vmr, &fault))) {
        mmap_unlock(mmap);
        goto send_SIGSEGV;
    }

    mmap_unlock(mmap);
done:
    popcli(); // get interrupts back online.
    return;

kernel_fault:
    if (fault.user)
        goto send_SIGSEGV;
    
    dump_tf(trapframe, 0);
    panic("page fault: %s:%ld: %s() @[\e[025453;04m0x%p\e[0m], err_code: %x, from '%s' space\n",
        __FILE__, __LINE__, __func__, fault.addr,
        fault.err_code, fault.user ? "user" : "kernel");
    goto done;

send_SIGSEGV:
    dump_tf(trapframe, 0);
    panic("page fault: %s:%ld: %s() @[\e[025453;04m0x%p\e[0m], err_code: %x, from '%s' space\n",
        __FILE__, __LINE__, __func__, fault.addr,
        fault.err_code, fault.user ? "user" : "kernel");
    goto done;

send_SIGBUS:
    dump_tf(trapframe, 0);
    panic("page fault: %s:%ld: %s() @[\e[025453;04m0x%p\e[0m], err_code: %x, from '%s' space\n",
        __FILE__, __LINE__, __func__, fault.addr,
        fault.err_code, fault.user ? "user" : "kernel");
    goto done;
}

int default_pgf_handler(vmr_t *vmr, vm_fault_t *fault) {
    int         err       = 0;
    uintptr_t   srcaddr   = 0;
    uintptr_t   reserv    = 0;
    uintptr_t   dstaddr   = 0;
    flags16_t   flags     = 0;      
    size_t      size      = 0;
    long        pgref     = 0;
    size_t      offset    = 0;
    page_t      *page     = NULL;
    char        buf[PGSZ] __aligned(PGSZ);

    if (vmr == NULL || fault == NULL)
        return -EINVAL;

    offset = (PGROUND(fault->addr) - __vmr_start(vmr)) + vmr->file_pos;

    // write access?
    if (fault->err_code & PTE_W) {
        if (!__vmr_write(vmr)) // region is not writable?
            return -EACCES;
        
        // region is writable

        if (fault->COW && (__vmr_shared(vmr) == 0)) { // copy-on-wrte if vmr is not shared.
            // reserve  this mapping incase we fail to COW.
            reserv  = fault->COW->raw;
            srcaddr = PGROUND(reserv);

            if ((pgref = __page_count(srcaddr)) > 1) {
                flags = PGOFF(fault->COW->raw) & ~PTE_ALLOC_PAGE; // mask PTE_ALLOC_PAGE flag.
                flags |= vmr->vflags;

                // remap this virtual address passing 'PTE_REMAPPG' flag to ensure a forced remap.
                if ((err = arch_map_n(fault->addr, PGSZ, flags | PTE_REMAPPG)))
                    return err;
                
                // copy from old physical address to new phys(via faulting vitual address).
                if ((err = arch_memcpypv(PGROUND(fault->addr), srcaddr, PGSZ))) {
                    arch_unmap_n(fault->addr, PGSZ);
#if defined (__x86_64__)
                    fault->COW->raw = reserv; // TODO: FIXME: need to send TLBSHTDWN.
                    // invalidate this page.
                    invlpg(fault->addr);

                    // send tlb shootdown to all cores.
                    arch_tlbshootdown(rdcr3(), fault->addr);
#endif
                    return err;
                }

                // relinquish a reference to this page frame address.
                __page_put(srcaddr);
            } else if (pgref == 1) { // we're the only ones using this page frame.
#if defined (__x86_64__)
                // enable write access.
                fault->COW->raw |= PTE_W;

                // invalidate this page.
                invlpg(fault->addr);

                // send tlb shootdown to all cores.
                arch_tlbshootdown(rdcr3(), fault->addr);
#endif
            } else {
                // TODO: send SIGSEGV(return -EFAULT)
                panic("page fault: %s:%ld: %s() @[\e[025453;04m0x%p\e[0m],"
                      " err_code: %x, from '%s' space\n",
                      __FILE__, __LINE__, __func__, fault->addr,
                      fault->err_code, fault->user ? "user" : "kernel");
            }
            return 0;
        }

        if (fault->err_code & PTE_P) {
            printk("page fault: %s:%ld: %s() @[\e[025453;04m0x%p\e[0m], err_code: %x, from '%s' space\n",
                  __FILE__, __LINE__, __func__, fault->addr,
                  fault->err_code, fault->user ? "user" : "kernel");
            return -EACCES;
        }

        if (vmr->file) { // vmr has backing store.
            ilock(vmr->file);
            size = (size_t)__min(PGSZ, (size_t)__min(vmr->filesz, igetsize(vmr->file) - offset));

            if (__vmr_shared(vmr)) { // shared vmr?
                if ((err = icache_getpage(vmr->file->i_cache, offset / PGSZ, &page))) {
                    iunlock(vmr->file);
                    return err;
                }
                
                page_incr(page);

                if ((err = arch_map_i(fault->addr, page_address(page), PGSZ, vmr->vflags))) {
                    iunlock(vmr->file);
                    return err;
                }
            } else { // vmr has no backing store.
                if ((err = arch_map_n(fault->addr, PGSZ, vmr->vflags))) {
                    iunlock(vmr->file);
                    return err;
                }
                
                if ((err = iread(vmr->file, offset, (void *)fault->addr, size)) < 0) {
                    arch_unmap_n(fault->addr, PGSZ);
                    iunlock(vmr->file);
                    return err;
                }
            }
            iunlink(vmr->file);
        } else { // No backing store, possibly an anonymous region.
            if ((err = arch_map_n(fault->addr,
                PGSZ, vmr->vflags | (__vmr_zero(vmr) ? PTE_ZERO : 0))))
                return err;
        }

        return 0;
    }

    if ((!__vmr_read(vmr) && !__vmr_exec(vmr))
        || (fault->err_code & PTE_P)) {
        printk("Inval access\n");
        return -EACCES;
    }

    if (vmr->file) {
        ilock(vmr->file);
        size = (size_t)__min(PGSZ, (size_t)__min(vmr->filesz, igetsize(vmr->file) - offset));

        if (igetsize(vmr->file) == 0) {
            iunlock(vmr->file);
            return -EFAULT;
        }

        if (__vmr_shared(vmr)) {
            if ((err = icache_getpage(vmr->file->i_cache, offset / PGSZ, &page))) {
                iunlock(vmr->file);
                return err;
            }

            page_incr(page);

            if ((err = arch_map_i(fault->addr, page_address(page), PGSZ, vmr->vflags))) {
                iunlock(vmr->file);
                return err;
            }
        } else {
            if ((dstaddr = pmman.get_page(GFP_NORMAL |
                (__vmr_zero(vmr) ? GFP_ZERO : 0))) == 0) {
                    iunlock(vmr->file);
                return -ENOMEM;
            }
            
            if ((err = iread(vmr->file, offset, buf, size)) < 0) {
                pmman.free(dstaddr);
                iunlock(vmr->file);
                return err;
            }

            if ((err = arch_memcpyvp(dstaddr, (uintptr_t)buf, PGSZ))) {
                pmman.free(dstaddr);
                iunlock(vmr->file);
                return err;
            }

            if ((err = arch_map_i(fault->addr, dstaddr, PGSZ, vmr->vflags))) {
                pmman.free(dstaddr);
                iunlock(vmr->file);
                return err;
            }
        }

        iunlock(vmr->file);
    } else {
        if ((err = arch_map_n(fault->addr, PGSZ,
            vmr->vflags) | (__vmr_zero(vmr) ? PTE_ZERO : 0))) {
            return err;
        }
    }

    return 0;
}