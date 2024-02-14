AC_INIT([Hyra], [1.2], [ian@osmora.org])

KERN_CFLAGS_X86_64="-fexceptions --std=gnu11 -ffreestanding -fno-stack-protector -fno-pic \\
	-Werror=implicit -Werror=implicit-function-declaration \\
	-Werror=implicit-int -Werror=int-conversion \\
        -Werror=missing-prototypes                   \\
	-Werror=incompatible-pointer-types -Werror=int-to-pointer-cast \\
	-Werror=return-type -Wunused -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow \\
	-mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel -pedantic \\
	-I sys/include/ -I sys/include/lib/ -D_KERNEL -Wno-pointer-sign -MMD"

KERN_CFLAGS_RISCV64="-fexceptions --std=gnu11 -ffreestanding -fno-stack-protector -fno-pic \\
	-Werror=implicit -Werror=implicit-function-declaration \\
	-Werror=implicit-int -Werror=int-conversion \\
        -Werror=missing-prototypes                   \\
	-Werror=incompatible-pointer-types -Werror=int-to-pointer-cast \\
	-Werror=return-type -Wunused -pedantic \\
	-I sys/include/ -I sys/include/lib/ -D_KERNEL -Wno-pointer-sign -MMD"


QEMU_FLAGS_X86_64="--enable-kvm -monitor stdio 	\\
		          -M q35 -m 1G -smp 4 -cpu host  \\
			      -cdrom Hyra.iso"

QEMU_FLAGS_RISCV64="-monitor stdio 	\\
			       -cdrom Hyra.iso -M virt -cpu rv64 -device qemu-xhci -device usb-kbd \\
                   -device ramfb -drive if=pflash,unit=0,format=raw,file=ovmf-riscv64/OVMF.fd"

HYRA_BUILDDATE=`export LANG=en_US.UTF-8 ; date`
HYRA_BUILDBRANCH="`basename $PWD`"

# Enable Spectre mitigation option
AC_ARG_ENABLE([spectre-mitigation],
    [AS_HELP_STRING([--enable-spectre-mitigation], [Enable Spectre mitigation (IBRS or similar)])],,
    [enable_spectre_mitigation=$enableval])

# Enable serial debug
AC_ARG_ENABLE([serial-debug],
    [AS_HELP_STRING([--enable-serial-debug], [Allow serial debugging])],,
    [enable_serial_debug=$enableval])

# Use enable RISCV64 port
AC_ARG_ENABLE([riscv64-port],
    [AS_HELP_STRING([--enable-riscv64-port], [Build Hyra for RISCV64])],,
    [enable_riscv64_port=$enableval])


if test "x$enable_spectre_mitigation" = "xyes"; then
    AC_SUBST(SPECTRE_MITIGATION, [1])
else
    AC_SUBST(SPECTRE_MITIGATION, [0])
fi

if test "x$enable_serial_debug" = "xyes"; then
    AC_SUBST(SERIAL_DEBUG, [1])
else
    AC_SUBST(SERIAL_DEBUG, [0])
fi

# Check which port to use
if test "x$enable_riscv64_port" = "xyes"; then
    AC_SUBST(ARCH, riscv64)
    AC_SUBST(KERNEL_CFLAGS, [$KERN_CFLAGS_RISCV64])
    AC_SUBST(QEMU, [qemu-system-riscv64])
    AC_SUBST(QEMU_FLAGS, [$QEMU_FLAGS_RISCV64])
else
    AC_SUBST(ARCH, amd64)
    AC_SUBST(QEMU, [qemu-system-x86_64])
    AC_SUBST(KERNEL_CFLAGS, [$KERN_CFLAGS_X86_64])
    AC_SUBST(QEMU_FLAGS, [$QEMU_FLAGS_X86_64])
fi

AC_SUBST(HYRA_BUILDDATE, [$HYRA_BUILDDATE])
AC_SUBST(HYRA_BUILDBRANCH, [$HYRA_BUILDBRANCH])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
