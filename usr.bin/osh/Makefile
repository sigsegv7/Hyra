include user.mk

CFILES = $(shell find . -name "*.c")

osh:
	$(CC) $(CFILES) -o $@ $(INTERNAL_CFLAGS)
