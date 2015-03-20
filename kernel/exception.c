#include <kernel/exception.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/klib.h>

static inline void setup_handle(uint8_t num, void *handle)
{
    idt_set_entry(num, GDT_FLAT_MEM_TEXT_SEL, handle, IDT_TYPE_INT, DPL_0);
}

void init_exception_handle()
{
    setup_handle(0, divide_by_zero_entry);
    setup_handle(1, debug_entry);
    setup_handle(2, non_maskable_int_entry);
    setup_handle(3, breakpoint_entry);
    setup_handle(4, overflow_entry);
    setup_handle(5, bound_range_exceeded_entry);
    setup_handle(6, invalid_opcode_entry);
    setup_handle(7, device_not_available_entry);
    setup_handle(8, double_fault_entry);
    setup_handle(10, invalid_tss_entry);
    setup_handle(11, segment_not_present_entry);
    setup_handle(12, stack_segment_fault_entry);
    setup_handle(13, general_protection_fault_entry);
    setup_handle(14, page_fault_entry);
    setup_handle(16, fp_exception_entry);
    setup_handle(17, alignment_check_entry);
    setup_handle(18, machine_check_entry);
    setup_handle(19, simd_fp_exception_entry);
    setup_handle(20, virtualization_entry);
    setup_handle(30, security_exception_entry);
}

static void divide_by_zero_handle()
{
    panic("Divide by zero fault");
}

static void debug_handle()
{
    panic("Debug trap");
}

static void non_maskable_int_handle()
{
    panic("Non maskable interrupt");
}

static void breakpoint_handle()
{
    panic("Breakpoint trap");
}

static void overflow_handle()
{
    panic("Overflow trap");
}

static void bound_range_exceeded_handle()
{
    panic("Bound range exceeded fault");
}

static void invalid_opcode_handle()
{
    panic("Invalid opcode fault");
}

static void device_not_available_handle()
{
    panic("Device not available fault");
}

static void double_fault_handle(uint32_t error_code)
{
    panic("Double fault abort, error code: 0x%x", error_code);
}

static void invalid_tss_handle(uint32_t error_code)
{
    panic("Invalid TSS fault, error code: 0x%x", error_code);
}

static void segment_not_present_handle(uint32_t error_code)
{
    panic("Segment not present fault, error code: 0x%x", error_code);
}

static void stack_segment_fault_handle(uint32_t error_code)
{
    panic("Stack segment fault, error code: 0x%x", error_code);
}

static void general_protection_fault_handle(uint32_t error_code)
{
    panic("General protection fault, error code: 0x%x", error_code);
}

static void page_fault_handle(void *virtual_address, uint32_t error_code)
{
    panic("Page fault exception at %p, error code: 0x%x",
          virtual_address, error_code);
}

static void fp_exception_handle()
{
    panic("Floating-point exception fault");
}

static void alignment_check_handle(uint32_t error_code)
{
    panic("Alignment check fault, error code: 0x%x", error_code);
}

static void machine_check_handle()
{
    panic("Machine check abort");
}

static void simd_fp_exception_handle()
{
    panic("SIMD floating-point exception fault");
}

static void virtualization_handle()
{
    panic("Virtualization exception fault");
}

static void security_exception_handle(uint32_t error_code)
{
    panic("Security exception, error code: 0x%x", error_code);
}

// Export to assembly
void *exception_handles[] =
{
    divide_by_zero_handle,
    debug_handle,
    non_maskable_int_handle,
    breakpoint_handle,
    overflow_handle,
    bound_range_exceeded_handle,
    invalid_opcode_handle,
    device_not_available_handle,
    double_fault_handle,
    NULL,
    invalid_tss_handle,
    segment_not_present_handle,
    stack_segment_fault_handle,
    general_protection_fault_handle,
    page_fault_handle,
    NULL,
    fp_exception_handle,
    alignment_check_handle,
    machine_check_handle,
    simd_fp_exception_handle,
    virtualization_handle,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    security_exception_handle,
    NULL
};
