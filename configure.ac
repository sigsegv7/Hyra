AC_INIT([Hyra], [1.2], [ian@osmora.org])

KERN_CFLAGS_X86_64="-fexceptions --std=gnu11 -ffreestanding -fno-stack-protector -fno-pic \\
	-Werror=implicit -Werror=implicit-function-declaration \\
	-Werror=implicit-int -Werror=int-conversion \\
        -Werror=missing-prototypes                   \\
	-Werror=incompatible-pointer-types -Werror=int-to-pointer-cast \\
	-Werror=return-type -Wunused -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow \\
	-mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel -pedantic \\
	-I sys/include/ -I sys/include/lib/ -D_KERNEL -Wno-pointer-sign -MMD"

QEMU_FLAGS_X86_64="--enable-kvm -monitor stdio 	\\
		          -M q35 -m 1G -smp 4 -cpu host  \\
			      -cdrom Hyra.iso"

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

AC_SUBST(HYRA_BUILDDATE, [$HYRA_BUILDDATE])
AC_SUBST(HYRA_BUILDBRANCH, [$HYRA_BUILDBRANCH])

AC_SUBST(KERNEL_CFLAGS, [$KERN_CFLAGS_X86_64])
AC_SUBST(QEMU_FLAGS, [$QEMU_FLAGS_X86_64])
AC_SUBST(QEMU, [qemu-system-x86_64])
AC_SUBST(ARCH, [amd64])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
