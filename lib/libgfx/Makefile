CFLAGS = -c -fno-stack-protector -nostdlib -static \
		 -Iinclude/ -I$(USRDIR)/include/
CFILES = $(shell find src/ -name "*.c")
OBJ = $(CFILES:.c=.o)

all: headers $(OBJ) build/libgfx.a
	echo $(USRDIR)
	mv build/libgfx.a $(USRDIR)/lib/
	cp -r include/ $(USRDIR)/include/

build/libgfx.a:
	mkdir -p build/
	ar rcs build/libgfx.a $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -Iinclude/ $< -o $@

.PHONY: headers
headers:
	cp -rf include/* $(USRDIR)/include/

.PHONY:
build/:
	mkdir -p build/

.PHONY: clean
clean:
	rm -f $(OBJ)
	rm -rf build/
