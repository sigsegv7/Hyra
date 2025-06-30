include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/bin/echo:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS)
