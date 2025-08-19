include user.mk

CFILES = $(shell find . -name "*.c")

$(ROOT)/base/usr/bin/screensave:
	gcc $(CFILES) -lgfx -o $@ $(INTERNAL_CFLAGS)
