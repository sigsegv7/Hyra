include user.mk

CFILES = $(shell find . -name "*.c")
CFLAGS = -Iinclude/

$(ROOT)/base/usr/bin/oasm:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS) $(CFLAGS)
