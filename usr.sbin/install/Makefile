include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/sbin/install:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS)
