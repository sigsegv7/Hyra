CC =
LIBDIR =
USRDIR =
LDSCRIPT =
INTERNAL_CFLAGS = -T$(LDSCRIPT) -znoexecstack -nostdlib -I$(USRDIR)/include/ $(USRDIR)/lib/libc.a
