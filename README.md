# jinOS

A distributed, AI-native operating system written from scratch in C++20.
One assistant, **Jin**, on all of the owner's machines; one kernel, **Siraj**,
on each of them. The design is in [docs/design.md](docs/design.md).

Status: **S0, foundations, in progress**. The S0 source is written but has
not been compiled or booted yet. The goal of S0 is for Siraj to boot through
Limine in QEMU (BIOS and UEFI), print over serial and on the framebuffer, and
be boot-tested by CI on every change.

## Building

Requirements on the development machine: `clang-21`, `lld-21`, `cmake`,
`ninja-build`, `qemu-system-x86`, `xorriso`, `mtools`, `gdb`, and the OVMF
firmware (`ovmf` package) for UEFI boots.

```
cmake -S . -B build -G Ninja
ninja -C build            # kernel ELF
ninja -C build image      # build/siraj.iso (BIOS + UEFI hybrid)
ninja -C build run        # boot it in QEMU (UEFI), serial on the terminal
ninja -C build run-bios   # same, BIOS firmware
ninja -C build check      # headless boot test, as CI runs it
```

`ninja -C build debug` starts QEMU stopped with a GDB stub on :1234;
`gdb -x tools/gdbinit` attaches to it.

## Layout

```
kernel/      Siraj: arch/x86_64, core
lib/kstd/    kernel support library (no standard library in the kernel)
boot/        Limine configuration
tools/       image builder, QEMU runner
tests/       boot tests
third_party/ Limine (BSD-2-Clause), the Limine protocol header, Terminus font (OFL)
docs/        design document, memory layout, decisions
```

## Licence

BSD-2-Clause (see [LICENSE](LICENSE)). Third-party code keeps its own
licence, recorded next to it under `third_party/`.
