ENTRY(_start)

PHDRS
{
    text          PT_LOAD    FLAGS((1 << 0) | (1 << 2)) ; /* Execute + Read */
    rodata        PT_LOAD    FLAGS((1 << 2)) ;            /* Read only */
    data          PT_LOAD    FLAGS((1 << 1) | (1 << 2)) ; /* Write + Read */
}

SECTIONS
{
    . = 0x10000;

    .text : {
        *(.text)
        *(.text.*)
    } :text

    .rodata : ALIGN(8) {
        *(.rodata)
        *(.rodata.*)
    } :rodata

    .data : ALIGN(8) {
        *(.data)
        *(.data.*)
    } :data

    .bss : ALIGN(8) {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        __bss_end = .;
    } :data
}
