CFLAGS = -c -fno-stack-protector -nostdlib -static -Iinclude/
LIBC_CFILES = $(shell find src/ -name "*.c")
LIBC_ASMFILES = $(shell find src/ -name "*.S")

LIBC_OBJ = $(LIBC_CFILES:.c=.o)
LIBC_ASMOBJ = $(LIBC_ASMFILES:.S=.S.o)

all: $(LIBC_OBJ) build/ld.so

build/ld.so: build/ linker/entry.c linker/entry.S
	mkdir -p $(@D)
	$(CC) $(CFLAGS) linker/entry.S -o build/entry.S.o
	$(CC) $(CFLAGS) linker/entry.c -o build/entry.o
	$(LD) build/entry.o build/entry.S.o -o $(LIBC_OBJ) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.S.o: %.S
	gcc $(CFLAGS) $< -o $@

.PHONY:
build/:
	mkdir -p build/

.PHONY: clean
clean:
	rm -rf build/
