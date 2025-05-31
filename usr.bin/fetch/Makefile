include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/bin/fetch:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS)
