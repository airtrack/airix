#include <mm/vmm.h>
#include <mm/pmm.h>

struct page_directory * vmm_alloc_vaddr_space()
{
    return CAST_PHYSICAL_TO_VIRTUAL(pmm_alloc_page_address());
}

void vmm_free_vaddr_space(struct page_directory *page_dir)
{
    physical_addr_t paddr = CAST_VIRTUAL_TO_PHYSICAL(page_dir);
    pmm_free_page_address(paddr);
}

struct page_table * vmm_alloc_page_table()
{
    return CAST_PHYSICAL_TO_VIRTUAL(pmm_alloc_page_address());
}

void vmm_free_page_table(struct page_table *page_tab)
{
    physical_addr_t paddr = CAST_VIRTUAL_TO_PHYSICAL(page_tab);
    pmm_free_page_address(paddr);
}

void vmm_map_page_table_index(struct page_directory *page_dir, uint32_t index,
                              struct page_table *page_tab, uint32_t flag)
{
    physical_addr_t paddr = CAST_VIRTUAL_TO_PHYSICAL(page_tab);
    page_dir->entries[index] = (pde_t)paddr | flag | VMM_PRESENT;
}

struct page_table * vmm_unmap_page_table_index(struct page_directory *page_dir,
                                               uint32_t index, uint32_t flag)
{
    physical_addr_t paddr = page_dir->entries[index] & 0xFFFFF000;
    struct page_table *page_tab = CAST_PHYSICAL_TO_VIRTUAL(paddr);
    page_dir->entries[index] = (flag & 0xFFF) & ~VMM_PRESENT;
    return page_tab;
}

void vmm_map_page_index(struct page_table *page_tab, uint32_t index,
                        physical_addr_t paddr, uint32_t flag)
{
    page_tab->entries[index] = (pte_t)paddr | flag | VMM_PRESENT;
}

physical_addr_t vmm_unmap_page_index(struct page_table *page_tab,
                                     uint32_t index, uint32_t flag)
{
    physical_addr_t paddr = page_tab->entries[index] & 0xFFFFF000;
    page_tab->entries[index] = (flag & 0xFFF) & ~VMM_PRESENT;
    return paddr;
}
