include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/bin/dmidump:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS)
