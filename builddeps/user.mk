CC =
LIBDIR =
USRDIR =
LDSCRIPT =
INTERNAL_CFLAGS = -T$(LDSCRIPT) -znoexecstack -nostdlib -I$(USRDIR)/include/ \
				  -L$(USRDIR)/lib -lc
