include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/bin/mex:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS)
