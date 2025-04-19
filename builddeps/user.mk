CC = gcc
LIBDIR =
USRDIR =
LDSCRIPT =
INTERNAL_CFLAGS = -T$(LDSCRIPT) -znoexecstack -nostdlib -I$(USRDIR)/include/ \
				  -L$(USRDIR)/lib -lc -pie -no-pie -fno-stack-protector \
				  -fno-asynchronous-unwind-tables
