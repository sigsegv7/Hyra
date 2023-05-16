AC_INIT([VegaOS], [0.0.1], [ian@mkall.org])

CFLAGS_X86_64="-fexceptions --std=gnu11 -ffreestanding -fno-stack-protector -fno-pic \\
	-Werror=implicit -Werror=implicit-function-declaration \\
	-Werror=implicit-int -Werror=int-conversion \\
        -Werror=missing-prototypes                   \\
	-Werror=incompatible-pointer-types -Werror=int-to-pointer-cast \\
	-Werror=return-type -Wunused -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow \\
	-mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel -pedantic \\
	-I sys/include/ -I sys/include/lib/ -D_KERNEL -Wno-pointer-sign -MMD"


CFLAGS_AARCH64="-fexceptions --std=gnu11 -ffreestanding -fno-stack-protector -fno-pic \\
	-Werror=implicit -Werror=implicit-function-declaration \\
	-Werror=implicit-int -Werror=int-conversion \\
        -Werror=missing-prototypes                   \\
	-Werror=incompatible-pointer-types -Werror=int-to-pointer-cast \\
	-Werror=return-type -Wunused -pedantic \\
	-I sys/include/ -I sys/include/lib/ -D_KERNEL -Wno-pointer-sign -MMD"

ARCH="x86_64"

AC_ARG_ENABLE([aarch64],
              AS_HELP_STRING([--enable-aarch64],
                              [Enable compilation for AARCH64 architecture @<:@default=no@:>@]]),
                              [aarch64_enabled="$enableval"],
                              [aarch64_enabled=""])

if test "x$aarch64_enabled" = "xyes"; then
    ARCH="aarch64"
    AC_SUBST(KERNEL_CFLAGS, [$CFLAGS_AARCH64])
else
    AC_SUBST(KERNEL_CFLAGS, [$CFLAGS_X86_64])
fi

AC_SUBST(ARCH, [$ARCH])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
