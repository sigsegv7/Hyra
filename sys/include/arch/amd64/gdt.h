#ifndef _AMD64_GDT_H_
#define _AMD64_GDT_H_

#include <sys/types.h>
#include <sys/cdefs.h>

#define KERNEL_CS 0x08
#define KERNEL_DS 0x10
#define GDT_TSS 5

struct __packed gdt_entry {
    uint16_t limit;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_hi;
};

struct __packed gdtr {
    uint16_t limit;
    uintptr_t offset;
};

__always_inline static inline void
gdt_load(struct gdtr *gdtr)
{
        __ASMV("lgdt %0\n"
               "push $8\n"               /* Push CS */
               "lea 1f(%%rip), %%rax\n"  /* Load 1 label address into RAX */
               "push %%rax\n"            /* Push the return address (label 1) */
               "lretq\n"                 /* Far return to update CS */
               "1:\n"
               "  mov $0x10, %%eax\n"
               "  mov %%eax, %%ds\n"
               "  mov %%eax, %%es\n"
               "  mov %%eax, %%fs\n"
               "  mov %%eax, %%gs\n"
               "  mov %%eax, %%ss\n"
               :
               : "m" (*gdtr)
               : "rax", "memory"
        );
}

extern struct gdt_entry g_gdt_data[256];

#endif  /* !AMD64_GDT_H_ */
