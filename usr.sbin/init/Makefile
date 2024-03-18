CC =
LD =
LDSO =

.PHONY: all
all:
	$(CC) -c main.c -o main.o;
	$(LD) --dynamic-linker /usr/lib/ld.so $(LDSO) main.o -o init -e main
