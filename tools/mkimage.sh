#!/bin/sh
# Builds a hybrid BIOS+UEFI ISO that boots Siraj through Limine.
# usage: mkimage.sh <siraj.elf> <limine.conf> <out.iso> <limine dir> <limine host tool>
set -eu
elf=$1; conf=$2; out=$3; limine=$4; limine_tool=$5

root=$(mktemp -d)
trap 'rm -rf "$root"' EXIT

mkdir -p "$root/boot/limine" "$root/EFI/BOOT"
cp "$elf" "$root/boot/siraj.elf"
cp "$conf" "$root/boot/limine/limine.conf"
cp "$limine/limine-bios.sys" "$limine/limine-bios-cd.bin" "$limine/limine-uefi-cd.bin" "$root/boot/limine/"
cp "$limine/BOOTX64.EFI" "$root/EFI/BOOT/"

xorriso -as mkisofs -quiet \
    -R -r -J \
    -b boot/limine/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table \
    -hfsplus -apm-block-size 2048 \
    --efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label \
    "$root" -o "$out"

"$limine_tool" bios-install "$out" >/dev/null 2>&1
