include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/bin/kstat:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS)
