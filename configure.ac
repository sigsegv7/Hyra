AC_INIT([Hyra], [1.6], [ian@osmora.org])

KERN_CFLAGS_AMD64="-fexceptions --std=gnu11 -ffreestanding -fno-stack-protector -fno-pic \\
	-Werror=implicit -Werror=implicit-function-declaration \\
	-Werror=implicit-int -Werror=int-conversion \\
        -Werror=missing-prototypes                   \\
	-Werror=incompatible-pointer-types -Werror=int-to-pointer-cast \\
	-Werror=return-type -Wunused -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow \\
	-mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel -pedantic \\
	-I sys/include/ -I sys/include/lib/ -D_KERNEL -Wno-pointer-sign -MMD \\
	-Wno-gnu-zero-variadic-macro-arguments -Wno-language-extension-token -Wno-tautological-constant-out-of-range-compare -Wno-c23-extensions"

QEMU_FLAGS_AMD64="--enable-kvm -monitor stdio 	\\
		          -M q35 -m 1G -smp 4 -cpu host  \\
			      -cdrom Hyra.iso"

HYRA_BUILDDATE=`export LANG=en_US.UTF-8 ; date`
HYRA_BUILDBRANCH="`basename $PWD`"

AC_SUBST(HYRA_BUILDDATE, [$HYRA_BUILDDATE])
AC_SUBST(HYRA_BUILDBRANCH, [$HYRA_BUILDBRANCH])

AC_SUBST(KERNEL_CFLAGS, [$KERN_CFLAGS_AMD64])
AC_SUBST(QEMU_FLAGS, [$QEMU_FLAGS_AMD64])
AC_SUBST(QEMU, [qemu-system-x86_64])
AC_SUBST(ARCH, [amd64])
AC_SUBST(TOOLCHAIN, [clang])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
