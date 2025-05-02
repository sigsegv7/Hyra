CFILES = $(shell find . -name "*.c")
CFLAGS = -pedantic
CC = gcc

.PHONY: all
all:
	mkdir -p bin/
	$(CC) $(CFLAGS) $(CFILES) -o bin/omar

.PHONY: clean
clean:
	rm -rf bin/
