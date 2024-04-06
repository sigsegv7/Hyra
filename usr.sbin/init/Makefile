include ../sbin.mk

.PHONY: all
all:
	@ # $(CC) -pie -nostdlib main.c -o init -e main
	$(CC) main.c -o init $(INTERNAL_CFLAGS)
