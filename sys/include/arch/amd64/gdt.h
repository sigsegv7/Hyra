#ifndef _AMD64_GDT_H_
#define _AMD64_GDT_H_

#include <sys/types.h>
#include <sys/cdefs.h>

#define GDT_TSS_INDEX 5
#define GDT_ENTRY_COUNT 7

/* Segment selectors */
#define KERNEL_CS 0x08
#define KERNEL_DS 0x10
#define USER_CS   0x18
#define USER_DS   0x20

/*
 * Bit definitions for regular segment descriptors
 *
 * See Intel SPG 3/25 Section 3.4.5 - Segment Descriptors
 */

#define GDT_ATTRIBUTE_ACCESSED     BIT(0)  /* Accessed */
#define GDT_ATTRIBUTE_EXECUTABLE   BIT(3)  /* Executable */
#define GDT_ATTRIBUTE_NONSYSTEM    BIT(4)  /* Code/data */
#define GDT_ATTRIBUTE_PRESENT      BIT(7)  /* Present */
#define GDT_ATTRIBUTE_64BIT_CODE   BIT(13) /* 64-bit code */
#define GDT_ATTRIBUTE_32BIT        BIT(14) /* 32-bit code/data */
#define GDT_ATTRIBUTE_GRANULARITY  BIT(15) /* 4KiB limit granularity */

/* Attributes for executable segments */
#define GDT_ATTRIBUTE_READABLE     BIT(1)  /* Readable */
#define GDT_ATTRIBUTE_CONFORMING   BIT(2)  /* Conforming */

/* Attributes for non-executable segments */
#define GDT_ATTRIBUTE_WRITABLE     BIT(1)  /* Writable */
#define GDT_ATTRIBUTE_EXPANDS_DOWN BIT(2)  /* See SPG 3/25 Section 6.8.1 */

/* DPL (Descriptor Privilege Level) specifier */
#define GDT_ATTRIBUTE_DPL0         0
#define GDT_ATTRIBUTE_DPL1         (1 << 5)
#define GDT_ATTRIBUTE_DPL2         (2 << 5)
#define GDT_ATTRIBUTE_DPL3         (3 << 5)

struct __packed gdt_entry {
    uint16_t limit;
    uint16_t base_low;
    uint8_t base_mid;
    uint16_t attributes;
    uint8_t base_hi;
};

struct __packed gdtr {
    uint16_t limit;
    uintptr_t offset;
};

extern struct gdt_entry g_gdt_data[GDT_ENTRY_COUNT];
extern const struct gdtr g_gdtr;

__always_inline static inline void
gdt_load(void)
{
    __ASMV("lgdt %0\n"
           "push %1\n"              /* Push code segment selector */
           "lea 1f(%%rip), %%rax\n" /* Load label 1 address into RAX */
           "push %%rax\n"           /* Push return address (label 1) */
           "lretq\n"                /* Far return to update CS */
           "1:\n"
           "  mov %2, %%ax\n"       /* Load data segment selectors */
           "  mov %%ax, %%ds\n"
           "  mov %%ax, %%es\n"
           "  mov %%ax, %%fs\n"
           "  mov %%ax, %%gs\n"
           "  mov %%ax, %%ss\n"
           :
           : "m" (g_gdtr), "i"(KERNEL_CS), "i"(KERNEL_DS)
           : "rax", "memory"
    );
}

#endif  /* !AMD64_GDT_H_ */
