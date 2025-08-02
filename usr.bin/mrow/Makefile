include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/bin/mrow:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS) -lgfx
