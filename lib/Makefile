LDSCRIPT =
USRDIR =
ROOT =
ARGS = -I$(ROOT)/builddeps LDSCRIPT=$(LDSCRIPT) USRDIR=$(USRDIR) ROOT=$(ROOT)

.PHONY: all
all:
	make -C libc/ $(ARGS)
	make -C libgfx/ $(ARGS)
	make -C liboda/ $(ARGS)
