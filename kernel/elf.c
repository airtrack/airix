#include <kernel/elf.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <string.h>

#define EI_MAG0 0x7F
#define EI_MAG1 'E'
#define EI_MAG2 'L'
#define EI_MAG3 'F'

#define ELF_ET_EXEC 2
#define ELF_PT_LOAD 1
#define ELF_PT_PHDR 6

struct elf_header
{
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct elf_prog_header
{
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
};

static bool load_from_prog_header(const char *elf_data, size_t size,
                                  const struct elf_prog_header *ph,
                                  struct process *proc)
{
    size_t file_size = ph->p_filesz;
    size_t file_offset = ph->p_offset;
    size_t page_offset = ph->p_vaddr & (PAGE_SIZE - 1);
    uint32_t vaddr_end = ph->p_vaddr + ph->p_memsz;

    if (ph->p_memsz < ph->p_filesz)
        return false;

    if (ph->p_offset >= size || ph->p_offset + ph->p_filesz > size)
        return false;

    /* This segment is empty, just returns true */
    if (ph->p_memsz == 0)
        return true;

    for (uint32_t vaddr = ph->p_vaddr - page_offset;
         vaddr < vaddr_end; vaddr += PAGE_SIZE)
    {
        size_t sz_in_page = PAGE_SIZE - page_offset;
        physical_addr_t paddr = pmm_alloc_page_address();
        void *dest = CAST_PHYSICAL_TO_VIRTUAL(paddr + page_offset);
        int extra_pages = 0;

        /* Out of memory, load fail */
        if (!paddr) return false;

        /* Fill zero in page */
        memset(CAST_PHYSICAL_TO_VIRTUAL(paddr), 0, PAGE_SIZE);

        /* Map memory page */
        if ((extra_pages = vmm_map(proc->page_dir, (void *)vaddr,
                                   paddr, VMM_WRITABLE | VMM_USER)) < 0)
        {
            pmm_free_page_address(paddr);
            return false;
        }

        proc->mem_pages += extra_pages + 1;

        if (file_size != 0)
        {
            /* Copy elf file content to destination */
            size_t copy_size = KMIN(file_size, sz_in_page);
            memcpy(dest, elf_data + file_offset, copy_size);
            file_offset += copy_size;
            file_size -= copy_size;
        }

        page_offset = 0;
    }

    return true;
}

static bool load_from_pht(const char *elf_data, size_t size,
                          const char *pht, uint32_t entry_size,
                          uint32_t num, struct process *proc)
{
    for (uint32_t i = 0; i < num; ++i, pht += entry_size)
    {
        const struct elf_prog_header *ph =
            (const struct elf_prog_header *)pht;

        if (ph->p_type == ELF_PT_LOAD ||
            ph->p_type == ELF_PT_PHDR)
        {
            if (!load_from_prog_header(elf_data, size, ph, proc))
                return false;
        }
    }

    return true;
}

bool elf_load_program(const char *elf_data, size_t size,
                      struct process *proc)
{
    const struct elf_header *header = (const struct elf_header *)elf_data;
    uint32_t phentsize = header->e_phentsize;
    uint32_t phnum = header->e_phnum;

    /* Check identification */
    if (header->e_ident[0] != EI_MAG0 ||
        header->e_ident[1] != EI_MAG1 ||
        header->e_ident[2] != EI_MAG2 ||
        header->e_ident[3] != EI_MAG3)
        return false;

    if (header->e_type != ELF_ET_EXEC)
        return false;

    if (header->e_phoff == 0 || header->e_phoff >= size)
        return false;

    if (header->e_phoff + phentsize * phnum > size)
        return false;

    if (!load_from_pht(elf_data, size, elf_data + header->e_phoff,
                       phentsize, phnum, proc))
        return false;

    proc->entry = header->e_entry;
    return true;
}
