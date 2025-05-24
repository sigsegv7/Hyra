AC_INIT([Hyra], [1.6], [ian@osmora.org])

TARGET="amd64"
PROJECT_ROOT=`pwd`
BOOT_FW="BOOTX64.EFI"
AC_ARG_ENABLE([aarch64],
              [AS_HELP_STRING([--enable-aarch64], [Enable AARCH64 Support])],
              [TARGET="aarch64"],
              []
)

KERN_CFLAGS_AMD64="-fexceptions --std=gnu11 -ffreestanding -fno-stack-protector -fno-pic \\
	-Werror=implicit -Werror=implicit-function-declaration \\
	-Werror=implicit-int -Werror=int-conversion \\
        -Werror=missing-prototypes                   \\
	-Werror=incompatible-pointer-types -Werror=int-to-pointer-cast \\
	-Werror=return-type -Wunused -mno-80387 -mno-mmx -mno-3dnow \\
        -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel -pedantic \\
	-Isys/include/ -I sys/include/lib/ -D_KERNEL -Wno-pointer-sign -MMD \\
    -Isys/include/dev/acpi/uacpi/ -nostdinc -Wno-format-pedantic \\
    -Wno-format-pedantic -Wno-gnu-zero-variadic-macro-arguments -Wno-c2x-extensions -Wno-language-extension-token -Wno-tautological-constant-out-of-range-compare"


KERN_CFLAGS_AARCH64="-fexceptions --std=gnu11 -ffreestanding -fno-stack-protector -fno-pic \\
	-Werror=implicit -Werror=implicit-function-declaration \\
	-Werror=implicit-int -Werror=int-conversion \\
        -Werror=missing-prototypes                   \\
	-Werror=incompatible-pointer-types -Werror=int-to-pointer-cast \\
	-Werror=return-type -Wunused -pedantic \\
	-I sys/include/ -I sys/include/lib/ -D_KERNEL -Wno-pointer-sign -MMD \\
	-Wno-format-pedantic -Wno-gnu-zero-variadic-macro-arguments -Wno-c2x-extensions \\
    -Isys/include/dev/acpi/uacpi/ -nostdinc -Wno-format-pedantic \\
    -Wno-language-extension-token -Wno-tautological-constant-out-of-range-compare"


QEMU_FLAGS_AMD64="--enable-kvm -monitor stdio 	\\
		          -M q35 -m 1G -smp 4 -cpu host  \\
			      -cdrom Hyra.iso"

KERN_CFLAGS="$KERN_CFLAGS_AMD64"

if test "x$TARGET" = "xaarch64"
then
    KERN_CFLAGS="$KERN_CFLAGS_AARCH64"
    BOOT_FW="BOOTAA64.EFI"
fi


HYRA_BUILDDATE=`export LANG=en_US.UTF-8 ; date`
HYRA_BUILDBRANCH="`basename $PWD`"

AC_SUBST(HYRA_BUILDDATE, [$HYRA_BUILDDATE])
AC_SUBST(HYRA_BUILDBRANCH, [$HYRA_BUILDBRANCH])

AC_SUBST(KERNEL_CFLAGS, [$KERN_CFLAGS])
AC_SUBST(QEMU_FLAGS, [$QEMU_FLAGS_AMD64])
AC_SUBST(QEMU, [qemu-system-x86_64])
AC_SUBST(BOOT_FW, [$BOOT_FW])
AC_SUBST(ARCH, [$TARGET])
AC_SUBST(PROJECT_ROOT, [$PROJECT_ROOT])
AC_SUBST(TOOLCHAIN, [clang])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
