include ../sbin.mk

.PHONY: all
all:
	@ # $(CC) -pie -nostdlib main.c -o init -e main
	$(CC) main.c mshell.c -o init $(INTERNAL_CFLAGS)
