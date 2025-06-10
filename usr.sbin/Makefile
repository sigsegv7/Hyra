LDSCRIPT =
USRDIR =
ROOT =
ARGS = -I$(ROOT)/builddeps LDSCRIPT=$(LDSCRIPT) USRDIR=$(USRDIR) ROOT=$(ROOT)

.PHONY: all
all:
	make -C init/ $(ARGS)
	make -C install/ $(ARGS)
