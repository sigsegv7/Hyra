ENTRY(_start)

SECTIONS
{
    . = 0x10000;

    .text :
    {
        *(.text)
        *(.text.*)
    }

    .data : ALIGN(8)
    {
        *(.data)
        *(.data.*)
    }

    .bss : ALIGN(8)
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        __bss_end = .;
    }

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}
