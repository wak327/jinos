#!/bin/sh
# Boots a Siraj ISO in QEMU.
# usage: run-qemu.sh <uefi|bios|debug|test> <iso> [extra qemu args...]
set -eu
mode=$1; iso=$2; shift 2

ovmf=${OVMF_CODE:-}
for f in /usr/share/OVMF/OVMF_CODE_4M.fd /usr/share/OVMF/OVMF_CODE.fd /usr/share/ovmf/OVMF.fd; do
    [ -n "$ovmf" ] && break
    [ -f "$f" ] && ovmf=$f
done

# Match what the design targets: q35, several CPUs, no KVM by default so a
# bug shows up the same way on every machine (set JINOS_KVM=1 to enable).
set -- \
    -machine q35 -m 1G -smp "${JINOS_SMP:-4}" -cpu "${JINOS_CPU:-max}" \
    -cdrom "$iso" -boot d \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
    -device virtio-rng-pci \
    -serial stdio \
    "$@"
[ "${JINOS_KVM:-0}" = 1 ] && set -- -accel kvm "$@"

case $mode in
    uefi)  [ -n "$ovmf" ] || { echo "run-qemu: OVMF not found" >&2; exit 2; }
           exec qemu-system-x86_64 -drive if=pflash,format=raw,readonly=on,file="$ovmf" "$@" ;;
    bios)  exec qemu-system-x86_64 "$@" ;;
    debug) exec qemu-system-x86_64 -s -S "$@" ;;
    test)  exec qemu-system-x86_64 -display none -no-reboot "$@" ;;
    *)     echo "run-qemu: unknown mode $mode" >&2; exit 2 ;;
esac
