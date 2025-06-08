include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/bin/time:
	gcc $(CFILES) -Iinclude/ -o $@ $(INTERNAL_CFLAGS)
