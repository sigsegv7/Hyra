include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/bin/readcore:
	gcc -Iinclude/ $(CFILES) -o $@ $(INTERNAL_CFLAGS)
