include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/sbin/inject:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS)
