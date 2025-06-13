LDSCRIPT =
USRDIR =
ROOT =
ARGS = -I$(ROOT)/builddeps LDSCRIPT=$(LDSCRIPT) USRDIR=$(USRDIR) ROOT=$(ROOT)

.PHONY: all
all:
	make -C osh/ $(ARGS)
	make -C kmsg/ $(ARGS)
	make -C fetch/ $(ARGS)
	make -C kfgwm/ $(ARGS)
	make -C time/ $(ARGS)
	make -C mex/ $(ARGS)
