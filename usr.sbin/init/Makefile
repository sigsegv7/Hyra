include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/sbin/init:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS)
