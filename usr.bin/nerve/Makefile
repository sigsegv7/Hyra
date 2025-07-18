include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/bin/nerve:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS)
