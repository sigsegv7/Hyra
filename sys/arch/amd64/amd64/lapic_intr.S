#include <machine/frameasm.h>

    .text
    .globl lapic_tmr_isr
lapic_tmr_isr:
    push_trapframe $0
    mov %rsp, %rdi
    call sched_switch
    call lapic_eoi
    pop_trapframe
    iretq
