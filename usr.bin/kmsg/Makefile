include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/bin/kmsg:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS)
