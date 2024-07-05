include user.mk

CFILES = $(shell find . -name "*.c")

init:
	gcc $(CFILES) -o $@ $(INTERNAL_CFLAGS)
