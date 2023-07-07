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

    .modules : {
        __modules_init_start = .; 
        *(.modules .modules)
        __modules_init_end = .; 
    } :rodata

    . += CONSTANT(MAXPAGESIZE);

    .data : {
        *(.data .data.*)
    } :data

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    } :data 

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}
