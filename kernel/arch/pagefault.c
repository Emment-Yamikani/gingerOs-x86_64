#include <arch/cpu.h>
#include <arch/paging.h>
#include <arch/ucontext.h>
#include <fs/inode.h>
#include <sys/thread.h>
#include <mm/mmap.h>
#include <mm/mm_zone.h>
#include <mm/pmm.h>
#include <mm/page.h>
#include <arch/signal.h>
#include <sys/system.h>
#include <sys/sysproc.h>
#include <mm/vmm.h>

#define panic_page_fault(trapframe, fault, type) ({                                                   \
    panic("page fault: %s:%ld: %s() @[\e[025453;04m0x%p\e[0m], err_code: %x : %s, from '%s' space\n", \
          __FILE__, __LINE__, __func__, fault->addr,                                                  \
          fault->err_code, type, fault->user ? "user" : "kernel");                                    \
})

int map_anonymous_page(vmr_t *vmr, vm_fault_t *fault) {
    int vflags = vmr->vflags | (__vmr_zero(vmr) ? PTE_ZERO : 0);
    /// Map an anonymous page (not backed by a file) into memory
    /// Map the anonymous page into the process's address space
    return arch_map_n(fault->addr, PGSZ, vflags);
}

int copy_page_on_write(vmr_t *vmr, vm_fault_t *fault, uintptr_t srcpaddr) {
    int err = 0;
    // virtual flags for vmr, maskout PTE_ALLOC_PAGE??
    int vflags = vmr->vflags | (PGOFF(fault->COW->raw) & ~PTE_ALLOC_PAGE);

    /// remap the page to a new location for COW
    /// vflags OR'ed with PTE_REMAPPG to force page remap.
    if ((err = arch_map_n(fault->addr, PGSZ, (PTE_REMAPPG | vflags)))) {
        return err;
    }

    // Perform the actual memory copy from the source to the destination
    if ((err = arch_memcpypv(PGROUND(fault->addr), PGROUND(srcpaddr), PGSZ))) {
        // If the copy fails, unmap the destination and restore the original COW mapping
        arch_unmap_n(fault->addr, PGSZ);
#if defined(__x86_64__)
        fault->COW->raw = srcpaddr; // Restore COW mapping
        invlpg(fault->addr);
        arch_tlbshootdown(rdcr3(), fault->addr);
        return err;
#endif
    }

    // Decrease reference count on the source page
    if ((err = __page_putref(PGROUND(srcpaddr)))) {
        // If the drop the ref on page fials, unmap the destination and restore the original COW mapping
        arch_unmap_n(fault->addr, PGSZ);
#if defined(__x86_64__)
        fault->COW->raw = srcpaddr; // Restore COW mapping
        invlpg(fault->addr);
        arch_tlbshootdown(rdcr3(), fault->addr);
        return err;
#endif
    }
    return 0;
}

int enable_write_access(vm_fault_t *fault) {
    // Enable write access to a COW page without copying
#if defined(__x86_64__)
    fault->COW->raw |= PTE_W;
    invlpg(fault->addr);
    arch_tlbshootdown(rdcr3(), fault->addr);
#endif
    return 0;
}

int load_page_from_file(vmr_t *vmr, vm_fault_t *fault, size_t offset, usize size) {
    int         err       = 0;
    uintptr_t   paddr     = 0;
    uint8_t     buf[PGSZ] = {0};
    page_t      *page     = NULL;

    // Load a page from a file into memory
    if (vmr->file) {
        ilock(vmr->file);
        if (igetsize(vmr->file) == 0) {
            iunlock(vmr->file);
            return -EFAULT;
        }

        /**
         * @brief get the minimum size to read from the file on-disk.
         * Take into account the size between the start of the memory region and
         * the faulting address. this TODO: must be subtracted from the __vmr_filesz(vmr),
         * but setting size to '0' if size is greater than __vmr_filesz(vmr) appears to work.
         */
        size = (size < __vmr_filesz(vmr)) ? 
                (size_t)__min(PGSZ, (size_t)__min(__vmr_filesz(vmr) - size,
                igetsize(vmr->file) - offset)) : 0;

        if (__vmr_shared(vmr)) { // shared vmr?
            if ((err = icache_getpage(vmr->file->i_cache, offset / PGSZ, &page))) {
                iunlock(vmr->file);
                return err;
            }

            if ((err = page_getref(page))) {
                iunlock(vmr->file);
                return err;
            }

            if ((err = page_get_address(page, (void **)&paddr))) {
                iunlock(vmr->file);
                return err;
            }

            if ((err = arch_map_i(fault->addr, paddr, PGSZ, vmr->vflags))) {
                iunlock(vmr->file);
                return err;
            }
        } else { // vmr is not shared.
            if ((err = arch_map_n(fault->addr, PGSZ, 
                vmr->vflags | (((__vmr_filesz(vmr) < __vmr_size(vmr)) ||
                    __vmr_zero(vmr)) ? PTE_ZERO : 0)))) {
                iunlock(vmr->file);
                return err;
            }
            
            if ((err = iread(vmr->file, offset, buf, size)) < 0) {
                arch_unmap_n(fault->addr, PGSZ);
                iunlock(vmr->file);
                return err;
            }

            memcpy((void *)PGROUND(fault->addr), buf, PGSZ);
        }

        iunlock(vmr->file);
        return 0;
    }

    return map_anonymous_page(vmr, fault);
}

// Handle a Copy-On-Write (COW) page fault
int handle_cow_fault(vmr_t *vmr, vm_fault_t *fault) {
    int         err         = 0;
    usize       pgref       = 0;
    uintptr_t   srcpaddr    = fault->COW->raw;
    
    if ((err = __page_getcount(PGROUND(srcpaddr), &pgref)))
        return err;

    if (pgref > 1) {
        // If the page is shared, copy it before writing
        return copy_page_on_write(vmr, fault, srcpaddr);
    } else if (pgref == 1) {
        // If the page is not shared, just mark it writable
        return enable_write_access(fault);
    } else {
        // Invalid page reference count
        return -EFAULT;
    }
}

int handle_writable_page_fault(vmr_t *vmr, vm_fault_t *fault, size_t offset, usize size) {
    int         err         = 0;
    uintptr_t   paddr       = 0;
    uint8_t     buf[PGSZ]   = {0};
    page_t      *page       = NULL;

    // Handle writable page faults for non-COW pages
    if (fault->err_code & PTE_P) {
        printk("%s:%d: page fault: faulting page is already present at addr %p, access: %x\n",
               __FILE__, __LINE__, fault->addr, fault->err_code);
        return -EFAULT;
    }

    if (vmr->file) {
        // Load the page from a file if it's backed by one
        ilock(vmr->file);
        /**
         * @brief get the minimum size to read from the file on-disk.
         * Take into account the size between the start of the memory region and
         * the faulting address. this TODO: must be subtracted from the __vmr_filesz(vmr),
         * but setting size to '0' if size is greater than __vmr_filesz(vmr) appears to work.
         */
        size = (size < __vmr_filesz(vmr)) ? 
                (size_t)__min(PGSZ, (size_t)__min(__vmr_filesz(vmr) - size,
                igetsize(vmr->file) - offset)) : 0;

        if (__vmr_shared(vmr)) { // shared vmr?
            if ((err = icache_getpage(vmr->file->i_cache, offset / PGSZ, &page))) {
                iunlock(vmr->file);
                return err;
            }
            
            if ((err = page_getref(page))) {
                iunlock(vmr->file);
                return err;
            }

            if ((err = page_get_address(page, (void **)&paddr))) {
                iunlock(vmr->file);
                return err;
            }

            if ((err = arch_map_i(fault->addr, paddr, PGSZ, vmr->vflags))) {
                iunlock(vmr->file);
                return err;
            }
        } else { // vmr is not shared.
            if ((err = arch_map_n(fault->addr, PGSZ, 
                vmr->vflags | (((__vmr_filesz(vmr) < __vmr_size(vmr)) ||
                    __vmr_zero(vmr)) ? PTE_ZERO : 0)))) {
                iunlock(vmr->file);
                return err;
            }
            
            if ((err = iread(vmr->file, offset, buf, size)) < 0) {
                arch_unmap_n(fault->addr, PGSZ);
                iunlock(vmr->file);
                return err;
            }

            memcpy((void *)PGROUND(fault->addr), buf, PGSZ);
        }
        iunlock(vmr->file);
        return 0;
    }

    // If the page is not backed by a file, map an anonymous page
    return map_anonymous_page(vmr, fault);
}

int handle_write_fault(vmr_t *vmr, vm_fault_t *fault, size_t offset, usize sz) {
    // Handle a write fault
    if (!__vmr_write(vmr)) {
        return -EACCES;  // Return error if the VMR is not writable
    }

    // Handle Copy-On-Write (COW) faults
    if (fault->COW && !__vmr_shared(vmr)) {
        return handle_cow_fault(vmr, fault);
    }

    // Handle other writable page faults
    return handle_writable_page_fault(vmr, fault, offset, sz);
}

int handle_read_exec_fault(vmr_t *vmr, vm_fault_t *fault, size_t offset, usize sz) {
    // Handle a read or execute fault
    if ((!__vmr_read(vmr) && !__vmr_exec(vmr))) {
        // Invalid access: neither read nor execute is allowed, or the page is already present
        printk("%s:%d: Invalid access: %x, start: %p\n", __FILE__, __LINE__, vmr->flags, __vmr_start(vmr));
        return -EACCES;
    }

    if (fault->err_code & PTE_P) {
        printk("%s:%d: page fault: faulting page is already present at addr %p, access: %x\n",
               __FILE__, __LINE__, fault->addr, fault->err_code);
        return -EFAULT;
    }

    // Load the page from the file if necessary
    return load_page_from_file(vmr, fault, offset, sz);
}

int default_pgf_handler(vmr_t *vmr, vm_fault_t *fault) {
    usize offset = 0;
    usize size   = 0;

    // Handle the default page fault processing.
    if (!vmr || !fault) {
        return -EINVAL;  // Return error if VMR or fault is invalid
    }

    // Calculate the offset within the file corresponding to the faulting address
    offset = (size = (PGROUND(fault->addr) - __vmr_start(vmr))) + vmr->file_pos;

    // If the fault was a write operation, handle it accordingly
    if (fault->err_code & PTE_W) {
        return handle_write_fault(vmr, fault, offset, size);
    }

    // Otherwise, handle read/execute faults
    return handle_read_exec_fault(vmr, fault, offset, size);
}

/// This function handles cases where the current thread is either
/// returning from a signal handler or needs to exit.
void handle_signal_or_thread_exit(mcontext_t *trapframe) {
    if (current_ishandling()) {
        arch_signal_return();
    } else if (current_ismain()) {
        //this exits the entire process.
        exit(trapframe->rax);  // Exit the main thread with the provided exit code
    } else {
        thread_exit(trapframe->rax);  // Exit the current thread
    }
}

void send_sigsegv(mcontext_t *trapframe, vm_fault_t *fault) {
    // Handle a SIGSEGV signal by dumping the trapframe and panicking
    dump_tf(trapframe, 0);
    /// For now just panic here,
    /// but functionality will be added to handle this appropriately.
    panic_page_fault(trapframe, fault, "SIGSEGV");
}

void send_sigbus(mcontext_t *trapframe, vm_fault_t *fault) {
    // Handle a SIGBUS signal by dumping the trapframe and panicking
    dump_tf(trapframe, 0);
    /// For now just panic here,
    /// but functionality will be added to handle this appropriately.
    panic_page_fault(trapframe, fault, "SIGBUS");
}

void handle_kernel_fault(mcontext_t *trapframe, vm_fault_t *fault) {
    // Handle page faults occurring in kernel mode
    if (fault->user) {
        // If the fault occurred in user space, send a SIGSEGV signal
        send_sigsegv(trapframe, fault);
    } else {
        // If the fault occurred in kernel space, dump the trapframe and panic
        dump_tf(trapframe, 0);
        panic_page_fault(trapframe, fault, "undef");
    }
}

int handle_vmr_fault(vmr_t *vmr, vm_fault_t *fault) {
    int err = 0;

    // If the VMR has a custom page fault handler, invoke it
    if (vmr->vmops && vmr->vmops->fault) {
        err = vmr->vmops->fault(vmr, fault);
    } else {
        // Otherwise, use the default page fault handler
        err = default_pgf_handler(vmr, fault);
    }

    return err;
}

void arch_do_page_fault(mcontext_t *trapframe) {
    int         err     = 0;
    vm_fault_t  fault   = {0};
    vmr_t       *vmr    = NULL;
    mmap_t      *mmap   = NULL;

    // Get the faulting address and error code
    fault.addr = rdcr2();
    fault.err_code = trapframe->errno;
#if defined(__x86_64__)
    // Determine if the fault occurred in user mode
    fault.user = x86_64_tf_isuser(trapframe);
#endif
    // Check if the faulting address is a Copy-On-Write (COW) page
    err = arch_getmapping(fault.addr, &fault.COW);

    if (current) {
        // Handle special cases where the trapframe's instruction pointer (RIP) indicates
        // that the thread is returning from a signal handler or should exit.
#if defined(__x86_64__)
        if (trapframe->rip == MAGIC_RETADDR) {
#endif
            handle_signal_or_thread_exit(trapframe);
        }

        /// TODO: increase refcnt on mmap here.
        mmap = current->t_mmap;  // Retrieve the current thread's memory map
    }

    // Handle kernel-mode faults or cases where the mmap is NULL
    if (mmap == NULL || iskernel_addr(fault.addr)) {
        handle_kernel_fault(trapframe, &fault);
        return;
    }

    // printk("PF: %p, cpu[%d, ncli: %d] tid[%d:%d], rip: %p\n",
        // fault.addr, getcpuid(), cpu->ncli, getpid(), gettid(), trapframe->rip);

    // Lock the memory map and find the corresponding virtual memory region (VMR)
    mmap_lock(mmap);
    if (NULL == (vmr = mmap_find(mmap, fault.addr))) {
        mmap_unlock(mmap);
        // If no VMR is found, send a SIGSEGV signal to the process
        send_sigsegv(trapframe, &fault);
        return;
    }

    // Handle the page fault within the found VMR
    if ((err = handle_vmr_fault(vmr, &fault)) == -EFAULT) {
        // Handle errors specific to SIGBUS or SIGSEGV signals
        send_sigbus(trapframe, &fault);
    } else if (err) {
        send_sigsegv(trapframe, &fault);
    }

    mmap_unlock(mmap);
}