# $Id$

AC_INIT([Vega], [0.0.1], [ian@vegaa.systems])

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
			      -cdrom Vega.iso"

VEGA_BUILDDATE=`export LANG=en_US.UTF-8 ; date`
VEGA_BUILDUSER=`whoami`
VEGA_BUILDHOST=`hostname`
VEGA_BUILDDIR=`pwd`

AC_SUBST(VEGA_BUILDDATE, [$VEGA_BUILDDATE])
AC_SUBST(VEGA_BUILDUSER, [$VEGA_BUILDUSER])
AC_SUBST(VEGA_BUILDHOST, [$VEGA_BUILDHOST])
AC_SUBST(VEGA_BUILDDIR, [$VEGA_BUILDDIR])

AC_SUBST(KERNEL_CFLAGS, [$KERN_CFLAGS_X86_64])
AC_SUBST(QEMU_FLAGS, [$QEMU_FLAGS_X86_64])
AC_SUBST(QEMU, [qemu-system-x86_64])
AC_SUBST(ARCH, [amd64])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
