include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/bin/date:
	gcc $(CFILES) -Iinclude/ -o $@ $(INTERNAL_CFLAGS)
