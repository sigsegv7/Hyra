OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(main)

PHDRS
{
    text          PT_LOAD    FLAGS((1 << 0) | (1 << 2)) ; /* Execute + Read */
    rodata        PT_LOAD    FLAGS((1 << 2)) ;            /* Read only */
    data          PT_LOAD    FLAGS((1 << 1) | (1 << 2)) ; /* Write + Read */
}

SECTIONS
{
    . = 0xFFFFFFFF80000000;

    .text : {
        *(.text .text.*)
    } :text

    . += CONSTANT(MAXPAGESIZE);

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    .drivers : {
        __drivers_init_start = .;
        *(.drivers .drivers)
        __drivers_init_end = .;
    } :rodata

    .drivers.defer : {
        __driversd_init_start = .;
        *(.drivers.defer .drivers.defer)
        __driversd_init_end = .;
    } :rodata

    . += CONSTANT(MAXPAGESIZE);

    .data : {
        *(.data)
    } :data

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    /* -- Cache line alignment -- */
    . = ALIGN(64);
    .data.cacheline_aligned : {
        *(.data.cacheline_aligned)
    }

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}
