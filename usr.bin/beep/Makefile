include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/bin/beep:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS)
