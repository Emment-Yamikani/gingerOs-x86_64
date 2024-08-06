#include <bits/errno.h>
#include <fs/fs.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <mm/mmap.h>
#include <sys/elf.h>
#include <sys/system.h>

int binfmt_elf_check(inode_t *binary) {
    Elf64_Ehdr h;

    if (binary == NULL)
        return -EINVAL;

    iassert_locked(binary);
    if (iread(binary, 0, &h, sizeof h) != sizeof h)
        return -EAGAIN;

    if ((h.e_ident[EI_MAG0] != 0x7f) || (h.e_ident[EI_MAG1] != 'E')||
        (h.e_ident[EI_MAG2] != 'L')  || (h.e_ident[EI_MAG3] != 'F'))
        return -EINVAL;

    if (h.e_ident[EI_CLASS] != ELFCLASS64 ||
        h.e_ident[EI_DATA]  != ELFDATA2LSB ||
        h.e_ident[EI_VERSION] != EV_CURRENT)
        return -EINVAL;
    return 0;
}

int binfmt_elf_load(inode_t *binary, mmap_t *mmap, thread_entry_t *entry) {
    int         err     = 0;
    int         prot    = 0;
    int         flags   = 0;
    size_t      memsz   = 0;
    Elf64_Ehdr  elf     = {0};
    Elf64_Phdr  *hdr    = NULL;
    vmr_t       *vmr    = NULL; 
    Elf64_Phdr  *phdr   = NULL;

    if (binary == NULL || entry == NULL)
        return -EINVAL;
    
    iassert_locked(binary);

    if ((err = iread(binary, 0, &elf, sizeof elf)) != sizeof elf)
        return err;

    if (NULL == (phdr = kmalloc(elf.e_phentsize * elf.e_phnum)))
        return -ENOMEM;

    if ((err = iread(binary, elf.e_phoff, phdr,
        elf.e_phentsize * elf.e_phnum)) !=
        (elf.e_phentsize * elf.e_phnum)) {
        printk("%s:%d: Failed in ELF\n", __FILE__, __LINE__);
        goto error;
    }

    for (size_t i = 0; i < elf.e_phnum; ++i) {
        hdr = &phdr[i];
    
        if (hdr->p_type == PT_LOAD) {
        
            // printk("elf_phdr[%d]: [%s] addr: %p, off: %p, memsz: %ld, filesz: %ld\n",
            //     hdr-phdr, (hdr->p_flags & 7) == 7 ? "rwx" :
            //     (hdr->p_flags & 7) == 6 ? "rw_" : (hdr->p_flags & 7) == 5 ? "r_x" :
            //     (hdr->p_flags & 7) == 4 ? "r__" : (hdr->p_flags & 7) == 3 ? "_wx" :
            //     (hdr->p_flags & 7) == 2 ? "_w_" : (hdr->p_flags & 7) == 1 ? "__x" : "___",
            //     hdr->p_vaddr, hdr->p_offset, hdr->p_memsz, hdr->p_filesz
            // );

            memsz = PGROUNDUP(hdr->p_memsz);
            prot  = (hdr->p_flags & PF_X ? PROT_X: 0)|
                    (hdr->p_flags & PF_W ? PROT_W: 0)|
                    (hdr->p_flags & PF_R ? PROT_R: 0);
            flags = MAP_PRIVATE | MAP_DONTEXPAND | MAP_FIXED;

            if ((err = mmap_map_region(mmap,
                ALIGN4K(hdr->p_vaddr), memsz, prot, flags, &vmr))){
                printk("%s:%d: Failed in ELF\n", __FILE__, __LINE__);
                goto error;
            }
            vmr->file       = binary;
            vmr->filesz     = hdr->p_filesz;
            vmr->memsz      = hdr->p_memsz;
            vmr->file_pos   = hdr->p_offset;
        }
    }

    // mmap_dump_list(*proc->mmap);

    kfree(phdr);
    *entry = (thread_entry_t)elf.e_entry;
    return 0;
error:
    if (phdr)
        kfree(phdr);

    mmap_clean(mmap);

    printk("error: %d occured while trying to load elf-file\n", err);
    return err;
}