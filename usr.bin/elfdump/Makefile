include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/bin/elfdump:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS)
