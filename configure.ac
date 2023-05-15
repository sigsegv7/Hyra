AC_INIT([VegaOS], [0.0.1], [ian@mkall.org])

CFLAGS_X86_64="-fexceptions --std=gnu11 -ffreestanding -fno-stack-protector -fno-pic \\
	-Werror=implicit -Werror=implicit-function-declaration \\
	-Werror=implicit-int -Werror=int-conversion \\
        -Werror=missing-prototypes                   \\
	-Werror=incompatible-pointer-types -Werror=int-to-pointer-cast \\
	-Werror=return-type -Wunused -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow \\
	-mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel -pedantic \\
	-I sys/include/ -I sys/include/libkern/ -D_KERNEL -Wno-pointer-sign -MMD"

AC_SUBST(KERNEL_CFLAGS, [$CFLAGS_X86_64])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
