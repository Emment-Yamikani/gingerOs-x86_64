#include <bits/errno.h>
#include <fs/fs.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <mm/mmap.h>
#include <sys/elf.h>
#include <sys/system.h>

// Global symbol table for dynamic linking (can be extended with a hash map for efficiency).
typedef struct symtab_entry {
    char *name;
    void *addr;
} symtab_entry_t;

#define MAX_SYMBOLS     1024
static symtab_entry_t   symb_tab[MAX_SYMBOLS];
static int              symb_cnt = 0;

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


// Register a symbol in the global symbol table.
int register_symbol(const char *name, void *addr) {
    if (symb_cnt >= MAX_SYMBOLS) {
        return -ENOMEM;
    }

    if (!(symb_tab[symb_cnt].name = strdup(name)))
        return -ENOMEM;

    symb_tab[symb_cnt].addr = addr;
    symb_cnt++;
    return 0;
}

// Lookup a symbol in the global symbol table.
void *resolve_symbol(const char *name) {
    for (int i = 0; i < symb_cnt; i++) {
        if (string_eq(symb_tab[i].name, name)) {
            return symb_tab[i].addr;
        }
    }

    return NULL;
}

// ELF Loader for Executables and Shared Libraries.
int binfmt_elf_load(inode_t *binary, mmap_t *mmap, thread_entry_t *entry) {
    int         err         = 0;
    u64         memsz       = 0;
    u64         rela_count  = 0;
    Elf64_Ehdr   elf        = {0};
    Elf64_Dyn   *dyn        = NULL;
    Elf64_Phdr  *phdr       = NULL;
    Elf64_Rela  *rela       = NULL;
    Elf64_Sym   *symtab     = NULL;
    char        *strtab     = NULL;

    if (binary == NULL || entry == NULL)
        return -EINVAL;

    iassert_locked(binary);
    mmap_assert_locked(mmap);

    // Read ELF Header.
    if ((err = iread(binary, 0, &elf, sizeof elf)) != sizeof elf)
        return err;

    // Allocate and read program headers.
    if (!(phdr = kmalloc(elf.e_phentsize * elf.e_phnum)))
        return -ENOMEM;

    if ((err = iread(binary, elf.e_phoff, phdr, elf.e_phentsize * elf.e_phnum)
        ) != (elf.e_phentsize * elf.e_phnum)) {
        printk("%s:%d: Failed to read program headers\n", __FILE__, __LINE__);
        goto error;
    }

    // Load segments.
    for (u64 i = 0; i < elf.e_phnum; ++i) {
        vmr_t       *vmr = NULL;
        Elf64_Phdr  *hdr = &phdr[i];

        if (hdr->p_type == PT_LOAD) {

            // printk("elf_phdr[%d]: [%s] addr: %p, off: %p, memsz: %ld, filesz: %ld\n",
            //     hdr-phdr, (hdr->p_flags & 7) == 7 ? "rwx" :
            //     (hdr->p_flags & 7) == 6 ? "rw_" : (hdr->p_flags & 7) == 5 ? "r_x" :
            //     (hdr->p_flags & 7) == 4 ? "r__" : (hdr->p_flags & 7) == 3 ? "_wx" :
            //     (hdr->p_flags & 7) == 2 ? "_w_" : (hdr->p_flags & 7) == 1 ? "__x" : "___",
            //     hdr->p_vaddr, hdr->p_offset, hdr->p_memsz, hdr->p_filesz
            // );

            memsz = PGROUNDUP(hdr->p_memsz);
            int prot  =(hdr->p_flags & PF_X ? PROT_X : 0) |
                       (hdr->p_flags & PF_W ? PROT_W : 0) |
                       (hdr->p_flags & PF_R ? PROT_R : 0);

            int flags = MAP_PRIVATE | MAP_DONTEXPAND | MAP_FIXED;

            if ((err = mmap_map_region(mmap, ALIGN4K(hdr->p_vaddr), memsz, prot, flags, &vmr))) {
                printk("%s:%d: Failed to map region. err: %d\n", __FILE__, __LINE__, err);
                goto error;
            }

            vmr->file       = binary;
            vmr->memsz      = hdr->p_memsz;
            vmr->filesz     = hdr->p_filesz;
            vmr->file_pos   = hdr->p_offset;
        } else if (hdr->p_type == PT_DYNAMIC) { // Allocate and read the dynamic section.
            u64 dyn_size = hdr->p_filesz;

            if (!(dyn = kmalloc(dyn_size))) {
                err = -ENOMEM;
                goto error;
            }

            if (iread(binary, hdr->p_offset, dyn, dyn_size) != (isize)dyn_size) {
                err = -EIO;
                goto error;
            }
        }
    }

    // mmap_dump_list(*proc->mmap);

    // Process dynamic section.
    if (dyn) {
        for (u64 i = 0; dyn[i].d_tag != DT_NULL; i++) {
            switch (dyn[i].d_tag) {
            case DT_SYMTAB:
                symtab      = (Elf64_Sym *)(elf.e_entry + dyn[i].d_un.d_ptr);
                break;
            case DT_STRTAB:
                strtab      = (char *)(elf.e_entry + dyn[i].d_un.d_ptr);
                break;
            case DT_RELA:
                rela        = (Elf64_Rela *)(elf.e_entry + dyn[i].d_un.d_ptr);
                break;
            case DT_RELASZ:
                rela_count  = dyn[i].d_un.d_val / sizeof(Elf64_Rela);
                break;
            }
        }
    }

    // Apply relocations.
    if (rela && symtab && strtab) {
        for (u64 i = 0; i < rela_count; i++) {
            void        *sym_addr   = NULL;
            Elf64_Rela  *rel        = &rela[i];
            Elf64_Sym   *sym        = &symtab[ELF64_R_SYM(rel->r_info)];
            void        *rel_addr   = (void *)(elf.e_entry + rel->r_offset);
            const char  *sym_name   = strtab + sym->st_name;

            if (!(sym_addr = resolve_symbol(sym_name))) {
                printk("Unresolved symbol: %s\n", sym_name);
                err = -EINVAL;
                goto error;
            }

            switch (ELF64_R_TYPE(rel->r_info)) {
            case R_X86_64_RELATIVE:
                *(Elf64_Addr *)rel_addr = elf.e_entry + rel->r_addend;
                break;
            case R_X86_64_GLOB_DAT:
            case R_X86_64_JUMP_SLOT:
                *(Elf64_Addr *)rel_addr = (Elf64_Addr)sym_addr;
                break;
            default:
                printk("Unsupported relocation type: %d\n", ELF64_R_TYPE(rel->r_info));
                err = -EINVAL;
                goto error;
            }
        }
    }

    kfree(dyn);
    kfree(phdr);
    *entry = (thread_entry_t)elf.e_entry;

    return 0;
error:
    if (phdr)
        kfree(phdr);
    if (dyn)
        kfree(dyn);
    mmap_clean(mmap);
    printk("error: %d occurred while trying to load ELF file\n", err);
    return err;
}