#include <mm/pmm.h>
#include <kernal/klib.h>

void init_pmm(memory_map_entry_t *entries, uint32_t num)
{
    display_char(20, 5, num + '0');
}
