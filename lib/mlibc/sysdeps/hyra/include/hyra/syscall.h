#ifndef _HYRA_SYSCALL_H_
#define _HYRA_SYSCALL_H_

#define SYS_debug   0
#define SYS_exit    1

__attribute__((__always_inline__))
static inline long syscall0() {
    volatile long ret;
    asm volatile("int $0x80" : "=a"(ret));
    return ret;
}

__attribute__((__always_inline__))
static inline long syscall1(uint64_t code, uint64_t arg0) {
    volatile long ret;
    asm volatile("int $0x80"
                  : "=a"(ret)
                  : "a"(code),
                    "D"(arg0) : "memory");
    return ret;
}

__attribute__((__always_inline__))
static inline long syscall2(uint64_t code, uint64_t arg0, uint64_t arg1) {
    volatile long ret;
    asm volatile("int $0x80"
                  : "=a"(ret)
                  : "a"(code),
                    "D"(arg0),
                    "S"(arg1)
                  : "memory");
    return ret;
}

__attribute__((__always_inline__))
static inline long syscall3(uint64_t code, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    volatile long ret;
    asm volatile("int $0x80"
                  : "=a"(ret)
                  : "a"(code),
                    "D"(arg0),
                    "S"(arg1),
                    "d"(arg2)
                  : "memory");
    return ret;
}

#define _GET_SYSCALL(a0, a1, a2, a3, name, ...) name

#define __syscall(...) \
    _GET_SYSCALL(__VA_ARGS__, syscall3, syscall2, syscall1, \
                              syscall0)(__VA_ARGS__)

#endif  /* !_HYRA_SYSCALL_H_ */
