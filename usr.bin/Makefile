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
	make -C date/ $(ARGS)
	make -C mex/ $(ARGS)
	make -C beep/ $(ARGS)
	make -C mrow/ $(ARGS)
	make -C elfdump/ $(ARGS)
	make -C cat/ $(ARGS)
	make -C getconf/ $(ARGS)
	make -C echo/ $(ARGS)
	make -C readcore/ $(ARGS)
	make -C login/ $(ARGS)
	make -C sleep/ $(ARGS)
	make -C kstat/ $(ARGS)
	make -C nerve/ $(ARGS)
	make -C whoami/ $(ARGS)
	make -C oasm/ $(ARGS)
	make -C oemu/ $(ARGS)
	make -C dmidump/ $(ARGS)
	make -C sysctl/ $(ARGS)
	make -C reboot/ $(ARGS)
	make -C screensave/ $(ARGS)
