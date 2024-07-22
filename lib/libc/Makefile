CFLAGS = -c -fno-stack-protector -nostdlib -static -Iinclude/
LIBC_CFILES = $(shell find src/ -name "*.c")
LIBC_ASMFILES = $(shell find src/ -name "*.S")

LIBC_OBJ = $(LIBC_CFILES:.c=.o)
LIBC_ASMOBJ = $(LIBC_ASMFILES:.S=.S.o)

all: headers $(LIBC_ASMOBJ) $(LIBC_OBJ) build/libc.a

build/libc.a: build/
	ar rcs build/libc.a $(LIBC_OBJ) $(LIBC_ASMOBJ)

%.o: %.c
	$(CC) $(CFLAGS) -Iinclude/ $< -o $@

%.S.o: %.S
	$(CC) $(CFLAGS) -Iinclude/ $< -o $@

.PHONY: headers
headers:
	mkdir -p include/sys/
	cp -f $(USRDIR)/include/sys/*.h include/sys
	cp -f include/*.h $(USRDIR)/include/

.PHONY:
build/:
	mkdir -p build/

.PHONY: clean
clean:
	rm -f $(LIBC_OBJ) $(LIBC_ASMOBJ)
	rm -rf build/
