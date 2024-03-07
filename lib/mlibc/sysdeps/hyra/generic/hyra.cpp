#include <stddef.h>
#include <sys/types.h>
#include <hyra/syscall.h>

namespace mlibc {
void sys_libc_log(const char *msg) {
    __syscall(0, (uintptr_t)msg);
}

int sys_anon_allocate(size_t size, void **pointer) {
    (void)size;
    (void)pointer;
    while (1);
}

void sys_libc_panic() {
    sys_libc_log("\n** MLIBC PANIC **\n");
    while (1);
}

int sys_tcb_set(void *pointer) {
    (void)pointer;

    while (1);
}

void sys_exit(int status) {
}

void sys_seek(int fd, off_t offset, int whence, off_t *new_offset) {
    (void)fd;
    (void)offset;
    (void)whence;
    (void)new_offset;

    while (1);
}

int sys_vm_map(void *hint, size_t size, int prot, int flags, int fd,
               off_t offset, void **window) {
    (void)hint;
    (void)size;
    (void)prot;
    (void)flags;
    (void)fd;
    (void)offset;
    (void)window;

    while (1);
}

int sys_vm_unmap(void *address, size_t size) {
    (void)address;
    (void)size;

    while (1);
}

int sys_anon_free(void *pointer, size_t size) {
    (void)pointer;
    (void)size;

    while (1);
}

int sys_clock_get(int clock, time_t *secs, long *nanos) {
    (void)clock;
    (void)secs;
    (void)nanos;

    while (1);
}

int sys_futex_wait(int *pointer, int expected, const struct timespec *time) {
    (void)pointer;
    (void)expected;
    (void)time;

    return 0;
}

int sys_futex_wake(int *pointer) {
    (void)pointer;

    return 0;
}

int sys_open(const char *filename, int flags, mode_t mode, int *fd) {
    (void)filename;
    (void)flags;
    (void)mode;
    (void)fd;

    while (1);
}

int sys_read(int fd, void *buf, size_t count, ssize_t *bytes_read) {
    (void)fd;
    (void)buf;
    (void)count;
    (void)bytes_read;

    while (1);
}

int sys_write(int fd, const void *buffer, size_t count, ssize_t *written) {
    (void)fd;
    (void)buffer;
    (void)count;
    (void)written;

    while (1);
}

int sys_close(int fd) {
    (void)fd;

    while (1);
}

}
