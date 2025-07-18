include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/bin/whoami:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS)
