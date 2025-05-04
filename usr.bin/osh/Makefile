include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/bin/osh:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS)
