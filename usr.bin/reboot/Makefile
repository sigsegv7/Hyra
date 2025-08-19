include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/bin/reboot:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS)
