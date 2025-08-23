set -e

ARCH=aarch64
OUTPUT=ovmf/ovmf-$ARCH.fd

mkdir -p ovmf/
curl -Lo $OUTPUT https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-code-$ARCH.fd
dd if=/dev/zero of=$OUTPUT bs=1 count=0 seek=67108864
