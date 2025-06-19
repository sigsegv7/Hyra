CFLAGS = -c -fno-stack-protector -nostdlib -static -Iinclude/ -D_OLIBC
LIBC_CFILES = $(shell find src/ -name "*.c")
LIBC_ASMFILES = $(shell find src/arch/$(ARCH) -name "*.S")

LIBC_OBJ = $(LIBC_CFILES:.c=.o)
LIBC_ASMOBJ = $(LIBC_ASMFILES:.S=.S.o)

all: sys/include/machine headers $(LIBC_ASMOBJ) $(LIBC_OBJ) build/libc.a

build/libc.a: build/
	ar rcs build/libc.a $(LIBC_OBJ) $(LIBC_ASMOBJ)
	mv $(LIBC_ASMOBJ) build/   # For initial bootstrapping
	mv build/crti.S.o build/crti.o
	mv build/*.o build/*.a $(ROOT)/base/usr/lib/

%.o: %.c
	$(CC) $(CFLAGS) -Iinclude/ $< -o $@

%.S.o: %.S
	$(CC) $(CFLAGS) -Iinclude/ $< -o $@

sys/include/machine:
	mkdir -p include/machine/
	cp include/arch/$(ARCH)/*.h include/machine/

.PHONY: headers
headers: sys/include/machine
	mkdir -p include/sys/
	cp -f $(USRDIR)/include/sys/*.h include/sys
	cp -f include/*.h $(USRDIR)/include/
	cp -f include/stdlib/*.h $(USRDIR)/include/
	cp -rf include/ousi $(USRDIR)/include/

.PHONY:
build/:
	mkdir -p build/

.PHONY: clean
clean:
	rm -f $(LIBC_OBJ) $(LIBC_ASMOBJ)
	rm -rf include/machine/
	rm -rf build/
