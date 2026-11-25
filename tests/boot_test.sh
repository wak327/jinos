#!/bin/sh
# Boots the selftest image headless under BIOS and UEFI firmware and with
# several CPU counts; passes when the kernel reports "S0 ok" and exits QEMU
# with the success code (33). As CI runs it.
set -u
iso=$1
run=$(dirname "$0")/../tools/run-qemu.sh
status=0

for fw in bios uefi; do
  for smp in 1 4; do
    log=$(mktemp)
    if [ "$fw" = uefi ]; then extra="-drive if=pflash,format=raw,readonly=on,file=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE_4M.fd}"; else extra=""; fi
    JINOS_SMP=$smp timeout 60 "$run" test "$iso" $extra >"$log" 2>&1
    code=$?
    if [ "$code" -eq 33 ] && grep -q "boot: S0 ok" "$log"; then
      echo "PASS  $fw smp=$smp"
    else
      echo "FAIL  $fw smp=$smp (exit $code)"; sed 's/^/      /' "$log"; status=1
    fi
    rm -f "$log"
  done
done
exit $status
