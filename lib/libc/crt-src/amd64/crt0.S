.text
.globl _start

_start:
    xor %rbp, %rbp      // End of callstack

    pushq %rsi           // argv
    pushq %rdi           // argc
    call __libc_init

    popq %rdi
    popq %rsi
    ud2                 // TODO: Call exit()
