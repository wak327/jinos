# Siraj memory layout (x86-64)

Status: S0. This records what the boot loader gives us; S1 replaces the
loader's page tables with our own and this file becomes the authority.

## At entry (Limine, base revision 4/5)

| Virtual range | What | Mapping |
|---|---|---|
| `0xffffffff80000000` + | the kernel ELF (`__kernel_start`…`__kernel_end`), 2 MiB-aligned physical base chosen by the loader | text RX, rodata R, data RW (write-back) |
| `hhdm_offset` + physical | direct map of memory-map regions of type usable, loader reclaimable, kernel + modules, framebuffer, ACPI (reclaimable, NVS), reserved-mapped | write-back; framebuffer write-combining (PAT5) |
| loader stack | 64 KiB, in loader-reclaimable memory | RW |

The lower half is unmapped. There is no identity map. Device registers are
not mapped; S1 maps them explicitly.

Addresses below 4 GiB are **not** guaranteed to be in the direct map
(revision 3+): only regions in the memory map are.

## Planned (S1)

- Kernel at `-2 GiB` as now (`-mcmodel=kernel`).
- Direct map at `0xffff800000000000`, usable RAM only, 1 GiB / 2 MiB pages.
- Device window at `0xffffc00000000000`, uncached, mapped on demand.
- Kernel stacks and per-CPU areas with guard pages at `0xffffe00000000000`.
- Physical zones: `<16 MiB` (legacy DMA), `<4 GiB` (32-bit devices), rest.
