# jinOS: design of a distributed, AI-native operating system

Status: draft 3 · Author: Waleed · 16 October 2026

> *Jin* (جن) is Arabic for a hidden being; the jin of the lamp is the
> powerful helper who lives in a small vessel and serves whoever holds it.
> jinOS is an operating system with a jin in it: an assistant in the spirit
> of JARVIS that knows its owner, acts for them, and runs on their own
> machines. There is one jin, however many lamps it lives in.

## Summary

jinOS is a new operating system written from scratch in C++20. It runs on
all of its owner's machines as **one system with one assistant**: Jin. Each
machine runs the jinOS kernel, **Siraj**, and joins the owner's **mesh**;
Jin's memory of the owner is replicated across the mesh, and work goes to
whichever machine suits it best. **Reflexes** are small learned policies,
with hand-written fallbacks, that tune decisions such as caching and
readahead (inside the kernel) or job placement (outside it) to how the owner
uses the machines; the large models that let Jin understand and act (its
**Mind**) run as ordinary programs. Jin acts only through a small, non-AI
**intent broker** that checks permissions, asks for confirmation and keeps a
journal. Existing software comes in through compatibility layers: Linux
programs in a lightweight virtual machine first, a Linux-compatible layer
inside jinOS growing alongside it, and Android apps later. The first target
is a cluster of PCs: QEMU virtual machines on the development PC, then two
older Intel laptops; phones come later.

This is a multi-year project for one developer. Section 15 gives the order
of work, the estimates and their basis, an intermediate release, and a
definition of v1.

---

## 1. Names

| Name | What |
|---|---|
| **jinOS** | The operating system as a whole |
| **Jin** | The assistant: the intelligence the owner talks to ("Hey Jin") |
| **Lamp** | Any machine running jinOS in the owner's mesh: a QEMU lamp, laptop 1, laptop 2 |
| **Siraj** (سراج, "lamp") | The kernel: it hosts Jin and keeps it contained |
| **Reflex** | A small learned policy built with the Reflex library, with a hand-written fallback and bounded actions. *In-kernel Reflexes* run inside Siraj; the others run in user-space services (section 8) |
| **Mind** | Jin's models: language, speech, embeddings (section 10) |
| **Mesh** | The owner's lamps working together (section 11) |
| **Broker** | The intent broker: the non-AI service through which Jin acts (section 13) |

An earlier, Debian-based jinOS lives in `~/personal/mobile-os`. It is kept
as it is and is not part of this design.

## 2. Vision

What the owner should experience, in order of importance (not of delivery;
see section 15):

1. **One Jin, every lamp.** The same assistant on every machine, with the
   same memory of the owner, able to continue on one machine a conversation
   started on another.
2. **The lamps work as one.** Each request goes to the machine best suited
   to it, and background jobs spread across all of them.
3. **It acts, safely.** Jin does things (sets reminders, finds files,
   drafts and sends messages, changes settings) only through permissions the
   owner granted, and asks first before anything risky.
4. **Private by construction.** Models run on the owner's machines; the
   owner's data and the traces the system learns from stay on them unless
   the owner sends them somewhere.
5. **It gets better at being yours.** The machines learn the owner's
   patterns and tune themselves to them.
6. **Honest about what it did.** Every action Jin takes can be inspected
   and, where possible, undone; every learned policy can be inspected and
   switched off.

## 3. Principles

Use these to settle disagreements.

1. **Safety beats intelligence.** Learned components only advise. Each one
   has a hand-written fallback and a bounded range of actions, so a bad
   model can make things slower but cannot break correctness or containment.
2. **No ambient authority.** A program can touch only what it was explicitly
   handed a capability for. This includes Jin and foreign software (Linux
   containers and virtual machines are given explicit capabilities, and what
   happens inside a VM is one unit as far as jinOS is concerned).
3. **Enforcement lives outside the AI.** Permissions, confirmations and the
   action journal are handled by small non-AI components that Jin cannot
   modify, imitate or bypass.
4. **Remote is never disguised as local.** A capability that points to
   another lamp is a different kind of thing: it is asynchronous, it can
   fail, and its failures are reported. Decades of distributed-systems
   experience show that hiding remoteness fails.
5. **Measure against the best baseline.** Every learned policy is judged
   against the strongest non-learned mechanism, on data it was not trained
   on.
6. **Bounded and checkable.** Interrupt handlers, code holding spinlocks and
   in-kernel Reflexes have bounded running times. Kernel models have fixed
   shapes, integer arithmetic, and a cost checked at compile time and
   measured on hardware.
7. **Local first.** Each lamp works on its own, offline. The mesh adds to it
   and is never required for basic use; content held only on another lamp
   is shown as such.
8. **One developer can understand all of it.** Prefer the simple design.
   Reuse well-licensed code (section 14) instead of rewriting it, and never
   hand-write cryptographic primitives.
9. **Start small, grow in rings.** First QEMU lamps and two laptops, then
   more PCs, then phones, then small devices.
10. **jinOS never holds the only copy.** Until v1 the owner's data is
    imported into jinOS, not moved; there is always a backup outside the
    mesh (section 11.8).

## 4. Non-goals

- A general replacement for Linux. Siraj supports a handful of chosen
  machines.
- A *single system image*: one kernel spanning machines, moving running
  processes between them, or sharing memory over the network. These were
  tried (LOCUS, Sprite, Amoeba, MOSIX) and proved fragile and costly. Each
  lamp runs its own kernel; the mesh is built from explicit messages.
- Making one answer faster by splitting a model across machines over Wi-Fi.
  Measured results show it is slower than one capable machine (section
  11.5).
- Porting a browser engine. Browsers come from Linux, unmodified (section
  12).
- Android apps in v1.
- GPU drivers in v1. Graphics are rendered in software.
- Running large models inside the kernel.
- A new programming language. New jinOS code is C++20 (plus minimal
  assembly); Python is used only for training and tools; reused libraries
  stay in their own language (mostly C).
- Multi-user and server use.

## 5. Decisions log

This section is the decisions log until a separate `docs/decisions.md`
replaces it.

| ID | Decision | Alternatives considered | Why |
|---|---|---|---|
| D1 | **x86-64 PCs first**: QEMU `q35` on the development PC (Intel i7-12700, nested KVM enabled), then two older Intel laptops. ARM64 phones later. CPU-specific code lives in `arch/` from day one. | ARM64 phones first | PCs boot through standard firmware, have wired Ethernet, hardware virtualization and documentation; the owner has two Intel laptops for a cluster. |
| D2 | **Hybrid kernel with capability objects.** Inside, everything is an object reached through a handle with rights (as in Fuchsia's Zircon). The first drivers run in the kernel but already talk to the rest of the system through shared-memory rings and channels, as a user-space driver would; they move out only where an IOMMU (Intel VT-d) with DMA remapping is enabled. | Unix-style monolithic kernel; pure microkernel | Capabilities are what contain Jin and foreign software; in-kernel drivers are quicker for one developer to start with. |
| D3 | **Linux-compatible through a personality.** Each container of Linux programs has a *personality instance* that implements Linux system calls on top of Siraj objects, as Fuchsia's Starnix does. It starts simple (each Linux system call forwarded as an exception) and moves to *restricted mode* (section 7.4) once correct and measured. It is kept thin and off v1's critical path, except for what the command-line shell needs (D12). | Personality inside the kernel (FreeBSD's Linuxulator, WSL1) | A bug in the huge Linux surface then compromises the Linux programs of one container, not the kernel. |
| D4 | **Linux GUI apps first through a compatibility VM** on PCs (Intel VT-x with EPT). v1 shows the guest's desktop as one window; per-app windows come later through a Wayland bridge device in the VMM. The VM tier is PC-only. | Personality only | Microsoft moved WSL from emulation to a real Linux kernel in a VM for compatibility and file-system speed (at the cost of slower file access across the boundary); a VM gives unmodified, fully sandboxed browsers years before the personality could. |
| D5 | **Jin is a user-space service**; every action goes through the broker, which checks permissions, shows confirmations on a trusted path and writes the journal. | Jin enforcing its own rules | A model steered by the text it reads must not be its own gatekeeper. |
| D6 | **Distribution in user space.** Siraj is a single-machine kernel. A mesh service on each lamp (`jin-meshd`) provides authenticated links, explicit remote capabilities and replication. Its protocol core is written without I/O so that it also runs in a deterministic host simulator. | Kernel-level distribution | Every system that hid remoteness in the kernel later had to expose it; user space can change without kernel changes. |
| D7 | **Where Reflexes live**: in the kernel only if the decision is taken inside a kernel path at microsecond-to-millisecond timescales (page-cache eviction, readahead, CPU frequency). Slower policies (app preload, job placement, sync timing) are user-space Reflexes using the same library. | All in the kernel; none in the kernel | Keeps sensitive, slow-moving data (which app, when, where) out of the kernel and its learned code small. |
| D8 | **C++20 for new code**; no new language. The kernel uses no exceptions and no RTTI; user-space code may use them where reused libraries need them (llama.cpp does). | Rust; a custom language | The owner's choice. C++ gives zero-cost abstractions and compile-time checks; freestanding C++ kernels exist (Zircon, SerenityOS). |
| D9 | **Wayland is the window protocol for foreign apps**, carried over Siraj channels with file descriptors mapped to handles (section 12.2). | One framebuffer per container; X11 | Browsers, GTK, Qt and Electron speak Wayland natively; ChromeOS uses this model for Linux and Android apps. |
| D10 | **Licence**: to be decided before the first commit (Q1). Reused code is chosen to be compatible with a permissive licence; GPL and LGPL code (for example waypipe, kvmtool, FriBidi) is avoided in jinOS itself. | | |
| D11 | **The Mind runs natively on jinOS**, built with the native SDK (section 14.2), so it can use model objects and the RAM arbiter. It never runs in the app VM. | Running it in the compatibility VM | A browser exploit in the VM must not reach the prompts, which contain the owner's memory. |
| D12 | **What "Linux-like" means**: Linux programs run unmodified (D3, D4); the everyday shell and command-line tools are Linux ones (BusyBox) running on the personality, which sees a `/` tree mapped onto jinOS storage; native jinOS programs use capabilities and are not POSIX programs. | A native jinOS shell | Familiar tools for the owner, no second shell to build. |
| D13 | **The compatibility VM is one protection domain**: one shared guest (with containers inside it for convenience), a fixed RAM cap, internet-only networking by default, and no capabilities to Jin's stores or keys. | One VM per app | Memory on old laptops; the VM boundary protects jinOS from the guest, not guest apps from each other. |
| D14 | **v1 storage**: the file system runs in the kernel behind the pager interface; the on-disk format is decided in Q5 (ext2, which standard tools can build and check, or a small log-structured format). Logs that must survive crashes (membership log, CRDT store, journal) use their own append-only, checksummed records with `fsync`, so their safety doesn't depend on the file system's. | A user-space file-system server in v1 | Simplest path; the first in-kernel Reflex needs an in-kernel page cache. |
| D15 | **System updates are signed** by a release key kept on a hardware token, never by mesh keys (section 13.9). | Unsigned images | Attacker 7 (section 13.1) includes the update path. |

## 6. Architecture

One lamp:

```
 ┌───────────────────────────── user space ─────────────────────────────┐
 │  jinOS apps    Compositor (Wayland) + trusted path    Home / launcher │
 │      │                      ▲                ▲                        │
 │  ┌───┴──── Jin ────────┐    │                │                        │
 │  │ Mind (native)       │    │   Linux containers (personality, D3)    │
 │  │ memory of the owner │    │   Compatibility VM (one guest, D4, D13) │
 │  │ planner             │    │                                         │
 │  └──────────┬──────────┘                                              │
 │             ▼                                                         │
 │   Broker ── trusted confirmation surface ── action journal            │
 │   jin-meshd · netd (network stack) · placement · trainer · model      │
 │   loader · model fetch · backup · drivers*                            │
 ├─────────────────────────────── Siraj ─────────────────────────────────┤
 │  objects & handles · scheduler · memory & page cache · file system    │
 │  IPC · restricted mode · VMX guests · random numbers · time           │
 │  in-kernel Reflexes · event recorder · weight check · model objects   │
 ├────────────────────────────── hardware ───────────────────────────────┤
 │  x86-64 (QEMU q35, Intel laptops) · later ARM64 phones                │
 └───────────────────────────────────────────────────────────────────────┘
   * drivers start in the kernel and move out where VT-d allows (D2)
```

The mesh:

```
   laptop 1 ◀──────── Ethernet (Wi-Fi later), Noise-encrypted ────────▶ laptop 2
      │  ╲                                                         ╱   │
      │   ╲   each lamp: own kernel, own keys, own Jin instance;   ╱    │
      │    ╲  memory replicated; requests placed on the best lamp ╱     │
   QEMU lamp ◀──────────────────────────────────────────────▶ QEMU lamp / later phones
          a relay the owner controls, for lamps on different networks (after v1)
```

## 7. Siraj, the kernel

### 7.1 Boot and platform (x86-64)

- **Boot loader: Limine** (BSD licence). It boots both UEFI and BIOS-only
  machines, loads Siraj as a higher-half ELF kernel, and hands over the
  memory map, a framebuffer, a pointer to the ACPI RSDP, loaded modules (for
  example CPU microcode and the initial RAM disk) and a partial higher-half
  direct map of RAM. Siraj requests a recent base revision, builds its own
  page tables and direct map in S1, and maps device registers explicitly.
- **Memory layout** (written down in `docs/memory-layout.md` before S1):
  user space in the lower half; kernel in the upper half; a direct map of
  RAM only, using 1 GiB and 2 MiB pages; a separate window for device
  registers mapped uncached; the framebuffer mapped write-combining through
  the PAT; a region for kernel stacks with guard pages; physical-memory zones
  for devices that can only address the low 4 GiB.
- **ACPI through uACPI** (MIT): table access in S1, the full namespace in
  S2, the embedded controller, battery, lid and power button after v1.
- **Interrupts and timers**: local APIC (x2APIC where present), I/O APIC
  with the legacy PIC masked, MSI/MSI-X preferred for PCI devices; TSC
  checked for invariance and synchronized across CPUs, calibrated from CPUID
  leaf 15h, then HPET, the ACPI PM timer or the PIT; the TSC-deadline mode of
  the APIC timer where available.
- **Multicore from the start**: all CPUs started in S1, per-CPU data,
  inter-processor interrupts, cross-CPU TLB shootdown, interrupt-disabling
  spinlocks, per-CPU run queues.
- **PCIe** through ACPI MCFG, with port-based configuration access as a
  fallback.

### 7.2 Entry and CPU hardening

The target laptops are older Intel machines, and most Intel CPUs made
before about 2019 are affected by Meltdown, L1TF and MDS.

- From S1: no-execute pages, write-protected kernel text, SMEP, SMAP, UMIP;
  separate interrupt stacks for NMI, double fault, machine check and debug.
- From S2: one audited API for copying to and from user memory, with fault
  recovery, fuzzed through the system-call interface; separate user and
  kernel page tables (KPTI, with PCID) on CPUs that report Meltdown; `VERW`
  on return to user mode where the microcode supports it; retpolines or
  enhanced IBRS; IBPB when switching between processes that don't trust each
  other.
- Early loading of Intel's published microcode, passed in as a Limine
  module.
- From the first VM: L1 data-cache flush on VM entry on L1TF-affected CPUs.
- SMT (hyper-threading) exposure is documented per laptop.

### 7.3 Objects, handles and rights

Kernel objects: process, thread, address space, memory object, channel
(messages with handle transfer), port (waiting on many things), event,
timer, interrupt, MMIO region, I/O-port range, DMA buffer (pinned memory
with device addresses and address-range constraints), guest and virtual CPU
(section 7.7), and job (a group of processes with a policy).

A **handle** is a number a process holds that names one object and says
what it may do with it (**rights**: read, write, map, execute, transfer,
duplicate, ...). Handles only lose rights when copied, never gain them.

**Revocation.** A handle cannot be taken back once given. Where revocation
is needed (Jin's access to a file, a remote session), the giver hands out a
handle to a **caretaker**: a small forwarding object it controls. Destroying
the caretaker cuts off every handle derived from it at once.

**Badges.** A badge is a value fixed into a handle by whoever mints it (a
server, the broker, `jin-meshd`); the kernel attaches it to every message
sent through that handle. The sender can neither choose nor change it, so it
reliably tells a server which client (or remote session) a call came from.

**Wait/wake.** A futex-style system call lets threads wait on and wake a
location, keyed by memory object and offset, so it works between processes
that share memory. Native code and the personality both use it.

**Memory-object seals**, such as "can't shrink" (the counterpart of Linux's
`F_SEAL_SHRINK`), let a process safely map memory another process gave it;
the compositor needs this for Wayland buffers (section 12.2).

### 7.4 Memory, scheduling and IPC

- Physical page allocator, then slab caches for kernel objects and
  size-class heaps. Allocation never throws: `try_make<T>()` returns
  `ErrorOr<OwnPtr<T>>`; plain `operator new` is not available in the kernel.
- Anonymous memory with demand-zero faults in S2; file-backed faults from
  the page cache in S6; copy-on-write of individual pages (reference-counted
  pages, copied on a write fault) for the personality's `fork` in S7.
- Cheap sparse reservations (browsers reserve gigabytes they never touch).
- **Epoch-based reclamation** (a lightweight read-copy-update), so data read
  without locks, such as Reflex weights, is freed only when no CPU can still
  be using it.
- Preemptible kernel; per-CPU run queues with load balancing; a deadline
  class for the compositor's frames.
- IPC over channels, with zero-copy transfer of memory objects.
- **Restricted mode** (after Fuchsia RFC-0261): a personality thread enters
  restricted mode explicitly; while in it, the thread sees only the lower,
  Linux part of its address space. A `syscall`, fault or kick traps into
  Siraj, which saves the registers into a state page bound to the thread and
  returns the thread to the personality's code in the upper region. That
  region is invisible to Linux code and shared by all processes of one
  container, which also share one handle table and one wait/wake context.
  Implementing it needs two page-table roots per process (tagged with PCIDs),
  checks on the return path (canonical addresses, user-settable flags only,
  `IRETQ` where `SYSRET` is unsafe) and the shared-region support above.
  There is one personality instance per container.

### 7.5 Drivers

Even while in the kernel, drivers use interrupt, MMIO, I/O-port and DMA
objects, and expose block and network devices as shared-memory rings plus a
channel (as virtio does). Moving one to user space then changes only where
it runs. **VT-d DMA and interrupt remapping** must be enabled before any
user-space driver or driver VM, and is enabled for all devices at boot where
the hardware has it, which also blocks DMA attacks through external ports.

Order:

1. QEMU: virtio block, network, console, input, random-number, GPU (2D).
2. Laptops, headless (S8): firmware framebuffer, keyboard (PS/2 through the
   i8042 controller, or USB HID through xHCI after taking it over from the
   firmware; EHCI on older machines), storage (AHCI SATA or NVMe), wired
   Ethernet (Intel e1000e or Realtek r8169, or a USB adapter if a laptop has
   no port), the CMOS real-time clock, ACPI basics.
3. Laptops, interactive (S12): the touchpad (PS/2, I2C-HID behind the
   chipset's I2C controller, or USB), backlight through ACPI.
4. After v1: Intel HD Audio, battery and lid (embedded controller), TPM 2.0
   (CRB for Intel's firmware TPM, TIS for discrete chips), Wi-Fi, webcams,
   sleep and resume, the Intel GPU. For Wi-Fi, one option is a **driver
   VM**: a tiny Linux guest that owns the card through VT-d passthrough and
   offers a virtual network card to jinOS, as Qubes OS does; it needs VT-d.

Which of these the laptops need depends on Q2.

### 7.6 Randomness and time

- **Random numbers**: a ChaCha20-based generator seeded from RDSEED/RDRAND
  where present, the TPM's generator once a TPM driver exists, virtio-rng in
  QEMU (every QEMU lamp gets `-device virtio-rng-pci`), timer and interrupt
  jitter, and a seed file replaced on every boot. Key generation waits until
  the generator is seeded. User space gets a `getrandom`-style system call.
  The kernel's stack-protector canary comes from it.
- **Time**: Siraj keeps monotonic time (from the TSC) separate from wall
  time. At boot it reads the real-time clock and raises it to a floor: the
  largest of the build time, the last time saved to disk and the largest
  hybrid-logical-clock value seen. Until synchronized (section 11.6) the wall
  clock is marked *unsynced*. A clock page lets user space read time without
  a system call.

### 7.7 Virtualization

Intel VT-x with EPT, VPID and unrestricted guest (the owner's laptops are
Intel; AMD-V can come later). Following Zircon, a **guest** object owns the
guest's memory mapping, and a **virtual CPU** object runs it.

- **In the kernel** (a "split irqchip", as in Fuchsia): VMX, EPT and VPID;
  emulation of the guest's local APIC (x2APIC registers and timer);
  decoding of the instruction behind each memory-mapped-I/O exit; doorbell
  traps for virtio queues that wake the VMM without stopping the virtual CPU;
  interrupt injection; filtering of CPUID and model-specific registers;
  switching the guest's extended FPU state; a stable guest clock.
- **In the user-space VMM**, one process per VM holding only that guest's
  handles: the guest's I/O APIC, every virtio device, and a minimal machine
  that boots a Linux kernel directly (no firmware) with virtio-mmio devices
  or virtio-pci with MSI.
- The VMM is written in C++ for jinOS, modelled on Fuchsia's VMM and Zircon's
  hypervisor (ported where licences allow). crosvm, Cloud Hypervisor and QEMU
  are built on Linux host interfaces and are not reused.
- Each virtio device's queue parser is fuzzed; devices handling untrusted
  data run in their own sandboxed process.
- A capability reports whether virtualization is available and, if not, why
  ("VT-x disabled in firmware", "no EPT").
- Development: VT-x code is tested under nested KVM on the development PC
  (QEMU's software emulation has no VT-x) and on the laptops. CI for it runs
  on a self-hosted runner with KVM.

### 7.8 Kernel C++ and support library

- Flags: `-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib
  -fno-threadsafe-statics -fno-omit-frame-pointer -fno-pic -fno-pie
  -mno-red-zone -mcmodel=kernel -mcx16 -fstack-protector-strong`, with the
  vector units disabled (`-mno-sse -mno-sse2 -mno-mmx -mno-avx -mno-80387`,
  as Linux does) so kernel code never touches floating-point or SIMD
  registers, and retpolines where needed (section 7.2). User threads do use
  those registers; their state (the `XSAVE` area) is saved and restored on
  every context switch.
- `-Wglobal-constructors` and `-Wexit-time-destructors` as errors: globals
  are `constinit` and initialized explicitly.
- **kstd**, the kernel library: `Array<T, N>`, `Span<T>`, `Ring<T, N>`,
  intrusive lists, fixed-point types; `RefPtr<T>`, `OwnPtr<T>`,
  `ErrorOr<T>`; `SpinLock`, `Mutex`, `LockGuard`; `PerCpu<T>`, whose
  accessor requires a guard that disables preemption; `move`, `forward`,
  `swap` (C++20's `<utility>` is not freestanding).
- C++ runtime support: `memcpy`/`memset`/`memmove`/`memcmp` in assembly,
  `__cxa_pure_virtual`, compiler-rt builtins built with the kernel flags.
- Allowed standard headers: the C++20 freestanding set only (`<cstdint>`,
  `<type_traits>`, `<concepts>`, `<atomic>`, `<new>`, `<limits>`, `<bit>`,
  `<compare>`, `<initializer_list>`), from a pinned header set.

### 7.9 Testing and debugging

- CI builds Siraj, boots it in QEMU without a display (BIOS and UEFI; 1, 2,
  4 and 8 CPUs; CPU models without x2APIC, XSAVE or PCID to match the
  laptops), checks the serial output and reports through QEMU's exit device.
- An in-kernel test runner; host builds of kstd under AddressSanitizer and
  UndefinedBehaviorSanitizer.
- Debug builds: undefined-behaviour sanitizer, slab red zones, poisoning and
  quarantine, then shadow-memory checking for use-after-free; a lock-order
  validator; checks on interrupt, preemption and may-sleep state; an NMI
  watchdog; TLB-shootdown and IPI torture tests.
- GDB attached to QEMU (`-s -S`); Bochs for debugging VT-x exits.
- Real hardware, which has no serial port: a panic screen drawn on the
  framebuffer with interrupts off and no allocation (from S0); a log ring at
  a fixed physical address, read back on the next boot when it survives; a
  polled network console over UDP (S8); network boot through Limine's PXE
  support, so a new build doesn't need a new USB stick.
- Fuzzers for every parser that takes outside data: system-call arguments,
  the weight checker, network packets, the mesh protocol, the Wayland
  protocol, virtio queues, model manifests.

## 8. Reflexes: small learned policies

### 8.1 Where they live and what they decide

Per D7, only decisions taken inside kernel paths are in-kernel Reflexes.
Learned policies have shipped in user space (Google's Adaptive Battery).
In-kernel learned policies have so far appeared in research prototypes:
LinnOS (a neural I/O-latency predictor in the block layer, 4–6 µs per
decision) and KML (readahead tuning inside Linux, up to 2.3× throughput).
jinOS's bet is to make such policies safe and permanent.

| Policy | Lives in | Decides | Strong baseline | How it is scored |
|---|---|---|---|---|
| Model-weight readahead / model-object cache | Kernel | Which weights to read ahead or keep in RAM | LRU/LFU within the RAM budget; the user-space expert cache (section 9.2) | Hit rate on the runtime's actual requests |
| Readahead | Kernel | How far ahead to read a file | Sequential readahead; KML | Prediction accuracy (were the read-ahead pages used?) |
| Page-cache eviction | Kernel | Which cached pages to drop first | CLOCK-Pro / multi-generation LRU-style rules | Refaults, counted with "ghost" lists of what each policy would have evicted |
| CPU frequency (CPUs without hardware-managed P-states) | Kernel | Frequency for the next interval | Utilization-based governor | Energy and missed deadlines, by randomized A/B time slices |
| App preload | User space | Which app opens next | Most-frequent app per hour and previous app (a count table) | Prediction hit rate |
| Job placement | User space | Which lamp runs a request | Cost model (section 11.5) | Latency and energy, by A/B |
| Sync timing | User space | When to replicate | Fixed rules (on mains power, on wired network) | Energy and staleness |

While apps run in the compatibility VM, the guest's own kernel makes their
caching decisions; Siraj's in-kernel Reflexes see only the VM's disk-image
I/O and native workloads such as the Mind's weight files.

### 8.2 Rules every in-kernel Reflex follows

1. **Fixed shape.** The model's architecture is fixed at compile time; only
   weights change at run time.
2. **Integer only.** Weights int8, accumulators int32, explicit scales and
   shifts; arithmetic saturates; a `static_assert` proves no accumulator can
   overflow.
3. **Budgets per kind of call site**, checked at compile time by a
   `constexpr` cost model *and* measured on the slowest target CPU:
   - hot paths (page fault, wakeup, every evicted page): at most ~500
     operations, about 1 µs, or a table lookup;
   - batch or periodic work (a reclaim pass, a 100 ms tick, a file open):
     at most ~20,000 operations.
   Heavy models run periodically and publish a small table that hot paths
   read. Every operation, including feature extraction, goes through
   Reflex-library types so the cost model counts it.
4. **No allocation, no blocking.** State is fixed-size, declared in the
   Reflex's type, and per-CPU or protected by a declared lock; `on()` may run
   on several CPUs at once.
5. **Bounded actions.** A Reflex only reorders or narrows the fallback's
   choices: it ranks the fallback's eviction candidates, picks a readahead
   size within a range, picks a frequency within a band around the
   fallback's. Any weights therefore have bounded harm: they can slow
   things, including one chosen app, but cannot break correctness.
6. **A fallback, required.** Every Reflex type provides `fallback()`; a C++20
   concept rejects types that don't. A watchdog switches back to it
   automatically (section 8.4).
7. **Traceable.** Every decision can be logged with its inputs and output.

### 8.3 What a Reflex looks like

```cpp
// Ranks the fallback's eviction candidates. Runs in the reclaim pass
// (a batch call site), never in the page-fault path.
struct EvictionOrder {
    using Net = Dense<int8_t, /*features*/ 16, /*hidden*/ 16, /*score*/ 1>;

    void rank(Span<PageInfo> candidates, const Net& net) {
        for (auto& p : candidates)                  // candidates.size() <= 64
            p.score = net(features(p))[0];
        sort_by_score(candidates);                  // only reorders: rule 5
    }

    void fallback(Span<PageInfo> candidates) {
        sort_by_age(candidates);                    // the plain policy
    }
};
static_assert(Policy<EvictionOrder>);               // has fallback(), int-only
static_assert(cost<EvictionOrder, /*max candidates*/ 64>() <= 20'000);
```

`Dense`, `Policy` and `cost` come from the **Reflex library**
(`kernel/reflex/`), header-only C++ that builds into the kernel, into
user-space services, and into host tools.

### 8.4 The learning loop

```
  kernel                          user space
 ┌────────────────┐  events   ┌──────────────┐   traces  ┌──────────────┐
 │ event recorder │──────────▶│ trace store  │──────────▶│   trainer    │
 │ (per-CPU rings)│           │ (encrypted)  │           │ (sandboxed,  │
 │ Reflex runs    │◀── weights via the reflex.update capability ──│ no network)  │
 └────────────────┘                                      └──────────────┘
```

1. **Record.** The kernel writes fixed-size event records (faults, refaults,
   readahead hits, file opens, CPU load) into per-CPU ring buffers.
2. **Store.** One service drains the rings into an encrypted local trace
   store that only the trainer can read (section 13.7).
3. **Train.** At first, models are trained in Python (PyTorch) on the
   development machine, from traces the owner exports after confirming
   (file paths hashed, apps reduced to slot numbers), and quantized to int8.
   Later, a C++ trainer using the Reflex library runs on the lamp while it
   is idle and on mains power.
4. **Load.** The kernel accepts weights only through a `reflex.update`
   capability held by the trainer. Weights from outside the lamp
   (development builds, shipped defaults) must be signed by a key the owner
   enrolled; the signature is checked by the service holding that
   capability, not by the kernel. A weight blob has a fixed size; the kernel
   copies it once into kernel memory, checks the model-shape hash and the
   ranges of scales and shifts, and only then uses it. This check proves
   memory safety; behavioural safety comes from rules 5 and 6.
5. **Evaluate.** A policy whose choices change what happens next can't be
   scored by replaying its predictions, because its imagined future diverges
   from the real one.
   - Prediction-style Reflexes (readahead, the model cache, app preload) run
     new weights in **shadow mode** first: their predictions are logged and
     scored against what actually happened next.
   - Eviction uses ghost lists: record what each policy would have evicted
     and count later accesses to those pages.
   - Frequency uses randomized A/B: a logged random 5–10% of time slices
     always run the fallback.
   - The watchdog compares the live Reflex with that randomized control, per
     app as well as overall, with a minimum sample count, a confidence
     threshold and hysteresis, and switches back on a regression.
6. **Promote and swap.** New weights are promoted only if the overall metric
   improves *and* no app regresses beyond a set margin. The swap is an
   atomic pointer change; the old weights are freed after an epoch (section
   7.4) and kept on disk for rollback.

### 8.5 How a Reflex is judged

- **Headroom first.** Before training anything, measure how much a perfect
  policy could gain: Belady's optimal eviction on recorded traces, cold
  versus warm start times for preload. If the headroom is small, stop and
  write the result up.
- **Baselines**: the strongest non-learned mechanism (section 8.1), a simple
  count-based learned model, and for readahead, KML's approach. A neural
  model is used only if it beats all of them.
- **Data**: at least four weeks of traces; train on earlier weeks, test on
  later ones.
- **Drift**: when accuracy falls below the fallback's, switch back and
  retrain. App identities use a stable slot table, so installing or removing
  apps doesn't silently change what an output means.

### 8.6 Where the first traces come from

Until Siraj runs real workloads, traces are recorded on ordinary Linux
machines (for example the development PC) with eBPF and replayed through the
trace simulator. This is a measurement lab only. A policy that wins there
must win again on Siraj, against Siraj's own fallback, on traces Siraj
records once the owner uses the laptops daily (S16).

### 8.7 The same code on the host

Reflexes are plain C++ with no kernel dependencies, so the same source
builds into the kernel, into a **trace simulator** that replays recorded
traces and reports the metric, and into unit tests under AddressSanitizer
and UndefinedBehaviorSanitizer. The simulator and the kernel must make
bit-identical decisions on the same trace.

## 9. Model memory: models larger than RAM

### 9.1 The limit

Generating one token needs every active weight once. Speed is bounded by the
slowest of three:

    tokens/s ≤ min( compute rate,
                    RAM bandwidth ÷ bytes read from RAM per token,
                    storage bandwidth ÷ (bytes per token × cache miss rate) )

and the wait before the first token is set separately, by **prefill**:
processing the prompt (system prompt, tool descriptions, retrieved memory),
which is compute-bound.

Consequences:

- A model that fits in RAM is the sweet spot: a dense model at 4-bit, or a
  mixture-of-experts (MoE) model whose weights fit. MoE models activate only
  some "experts" per token; a 30B-total, ~3B-active model reads ~1.5 GB per
  token at 4-bit if nothing is cached.
- Streaming from storage helps only when most active weights are already in
  RAM: the RAM budget and the hot-set hit rate are the real levers. A hard
  disk is far too slow; a SATA SSD reads about 0.5 GB/s; NVMe a few GB/s.
- The high (~90%) neuron sparsity that "LLM in a Flash" and PowerInfer
  exploit needs ReLU-family models trained or fine-tuned for it.
  Training-free methods (TEAL) reach about 40–50% on mainstream models,
  which still reads about half the weights per token, in scattered pieces,
  so it does not make flash streaming cheap. Storage reads work best in
  expert-sized chunks (around 1 MB or more).
- What jinOS can run depends on the laptops' RAM, CPUs and disks (Q2).

### 9.2 Model objects

- A sandboxed user-space **model loader** parses the model file and its
  manifest (tensors, layers, experts, which parts are always needed) and
  gives the kernel a fixed-format list of file extents, which the kernel
  bounds-checks. The kernel never parses model files.
- Model files are stored in the layout the compute code uses, so mapping
  them needs no repacking or copying; large mappings use 2 MiB pages.
- The inference runtime sends **hints** ("layer 12 next", "experts 4 and 17
  next"); the kernel schedules reads ahead of need and keeps always-needed
  regions pinned.
- One **memory arbiter** divides RAM between the model cache, the page cache,
  application memory and the compatibility VM (which has a fixed cap and
  returns free memory through virtio-balloon free-page reporting). Siraj has
  no swap, so VM memory is a hard commitment.
- **Gate:** first build the same features in user space (a memory-mapped
  file plus an explicit expert cache with direct I/O). The kernel object goes
  ahead only if it measurably beats that.

### 9.3 Compute

CPU inference first: ggml chosen at run time for the CPU's vector support
(AVX2 if present, otherwise AVX or SSE4). GPU work needs a GPU driver, which
is after v1.

## 10. Jin

### 10.1 Parts

- **Mind**: speech recognition, a language model, text-to-speech and an
  embedding model. The runtime is ggml/llama.cpp, compiled natively for jinOS
  with the native SDK (D11). Models are sized to each lamp, except the
  embedding model, which is the same on every lamp so that vectors are
  comparable (section 11.4).
- **Memory of the owner**: notes, files, contacts, calendar, conversations;
  indexed locally, replicated across the mesh (section 11.4), and labelled by
  where each item came from (section 13.4).
- **Planner**: turns requests into intents.
- **Presence**: Jin's text (and later voice) surfaces on each lamp.

### 10.2 Latency

Target: under 2 seconds to the first word for common commands.

- A tiny intent-and-slot model (milliseconds) handles fixed commands (set a
  reminder, find a file) without the language model.
- The fixed part of the prompt (system prompt, intent list) is processed
  once, and its key-value cache saved to disk and restored per request.
- Retrieved context is kept short.
- Long or heavy requests go to the strongest lamp (section 11.5).

### 10.3 One Jin, many lamps

Every lamp runs its own Jin instance, all sharing the owner's replicated
memory. "One Jin" is made explicit rather than magical.

- **Inference is placed; effects are not.** A request may be computed on
  another lamp, but the resulting proposal returns to the lamp where the
  owner is, and that lamp's broker confirms it (section 13.3).
- **Side effects at most once, never silently.**
  - Each action with an effect outside jinOS (sending a message, an email)
    has one **executor**, fixed when the action is created: the lamp holding
    the capability for that effect. It is recorded, with an idempotency key
    and an assignment epoch, in the action's replicated record.
  - Off-device effects never fail over automatically. If the executor can't
    be reached when the action is due, other lamps show "pending on laptop
    1". The owner may reassign it on a trusted surface, after the warning
    "may happen twice if laptop 1 is still running"; reassignment raises the
    epoch, and an executor acts only if it holds the newest epoch it has
    seen.
  - A call whose outcome is unknown (the link died mid-call) is reported as
    *unknown*, never silently retried.
  - Local notifications, such as a reminder alert, are deliberately
    at-least-once: any awake lamp may show one, and dismissal syncs.
- **Focus is advisory.** When lamps are connected, the one where the owner
  is active (or, for a wake word later, the one with the strongest
  detection) answers; when they are cut off, both may answer. Focus never
  decides side effects.
- **Conversations** are append-only logs, so if two lamps continue the same
  conversation while cut off, the result shows as two branches rather than
  interleaved turns.
- **Handoff** carries a small activity record (what the owner was doing,
  with references to replicated content), not a running process.

### 10.4 Acting in apps

- **jinOS apps** publish intents. v1 has three: `note.create`, `file.find`
  and `reminder.set`, implemented by jinOS's own notes, files and reminder
  services.
- **Linux apps** describe their user interface through the Linux
  accessibility interface (AT-SPI), which GTK and Qt apps support. After v1,
  a guest-side agent exposes the tree of one app the owner opted in, through
  the broker; everything read this way is untrusted (section 13.4), and
  actions become per-app broker intents, each confirmed. Browsers build that
  tree only when an assistive technology is attached, which costs CPU and
  memory.
- **Web apps and WhatsApp**: Jin cannot read or send WhatsApp messages in v1.
  The first route after v1 is a guest notification bridge (section 12.4);
  the AT-SPI bridge follows.

### 10.5 What Jin knows in v1

Notes and files created on the lamps; contacts and calendar imported once
from vCard and iCalendar exports; Jin's own conversation history; and app
usage as the compositor sees it (which window has focus, including the VM's
window).

## 11. The mesh: one Jin, many lamps

### 11.1 Identity and membership

- **Owner recovery key**, created at first install and kept offline (printed,
  or split among trusted places). It outranks every lamp and is used only to
  recover or override.
- **Device keys**: each lamp generates its own signing and key-agreement keys
  and never shares its private keys. In v1 they are stored wrapped by a key
  derived from the owner's passphrase (Argon2id). TPM sealing comes after v1
  where a TPM exists (TPM 2.0 chips don't implement the Ed25519/X25519
  curves, so the TPM seals a software key rather than holding it).
- **Membership log**: a signed causal graph of entries (add lamp, remove
  lamp, new key epoch, grant), each referencing every head its author knew.
  Lamps merge entries as a set, and membership is derived from the set by
  fixed rules, so no legitimate entry is ever discarded.
  - Adding or removing a lamp requires the owner's presence on the issuing
    lamp (unlock plus confirmation on its trusted surface).
  - Every add is announced on all other lamps with a one-step veto, and keys
    are wrapped for the new lamp only after a cooling-off period or a second
    lamp's approval. A compromised member could otherwise admit a device;
    the veto is the defence, and this limit is stated plainly.
  - A removal sticks: a removed lamp returns only through the recovery key
    or a fresh pairing from a remaining member.
  - If two lamps remove each other while cut off, both are suspended until
    the owner settles it on a trusted surface (or with the recovery key).
- **Device certificates** last 30 days and are a cache of the log, which is
  authoritative. Lamps exchange log heads at the start of every session,
  before anything else. A lamp with a screen renews only if the owner
  unlocked it within the period; a lamp without one (a QEMU lamp) renews
  when the owner approves it on a member lamp. A lamp whose certificate
  expired but which is still in the log rejoins through a handshake the owner
  confirms. Expiry gates mesh traffic only, never local use.
- **Pairing** in v1: a six-digit code shown on the new lamp's console and
  typed on an existing lamp, through a password-authenticated key exchange
  (SPAKE2+, RFC 9383, as Matter uses for numeric setup codes; or CPace),
  limited to a few tries. QR pairing comes after v1 (it needs a webcam
  driver).

### 11.2 Transport

- **Noise protocol** sessions between lamps (the protocol WireGuard uses):
  the `IK` pattern for known lamps, `XX` for first contact; rekeyed
  regularly; silent to unauthenticated packets. Only the Noise state
  machine is written in C++; the primitives come from a vetted library
  (section 14.1), and the implementation is tested against published Noise
  test vectors. State-changing calls never ride in the first, replayable
  message.
- A **link** is the authenticated session, not a network path: it survives
  address and path changes and rekeys, and breaks only on timeout or when a
  lamp restarts (detected by an incarnation number in the handshake).
- **Discovery** on the local network with DNS-SD names that are opaque and
  change every few minutes (an HMAC of a discovery key, rotated with each key
  epoch, and the time), accepting the neighbouring time slots too, and
  falling back to known addresses.
- **Lanes**: callers state what they need (bandwidth, latency, cost); bulk
  transfers wait for fast, unmetered links and mains power. v1 uses wired
  Ethernet on one network.
- **After v1**: a relay the owner controls, for lamps on different networks,
  forwarding traffic it cannot read (published NAT-traversal success is
  roughly 70–90%, so up to about 30% of connections between networks need
  it); hybrid post-quantum key exchange (ML-KEM).
- **Versions**: lamps agree on the highest common protocol version during
  the handshake; every stored record carries a format version; schemas only
  add fields, and unknown fields are kept. The membership log fails closed:
  a lamp that meets an entry it can't parse stops accepting membership
  changes past it and asks to be updated.

### 11.3 Remote capabilities

`jin-meshd` on each lamp turns local capabilities into remote ones, in the
style of Cap'n Proto RPC and the E language:

- A remote reference is its own type; every call on it is asynchronous and
  returns a promise; **promise pipelining** lets dependent calls go in one
  round trip.
- **Fail-stop**: when a link breaks, every reference over it breaks for good.
  Reconnecting uses a **sturdy ref**, a durable token bound to the holder's
  device key and the membership epoch, stored encrypted, with an expiry, and
  invalidated when the holder is removed or the caretaker behind it is
  destroyed.
- Each remote method's interface declares it *pure* (safe to replay) or
  *effectful*; only pure calls are ever replayed.
- Rights are cut down in the proxy on the hosting lamp; references aren't
  passed on to a third lamp by default.
- Placement cancels jobs on a removed lamp and rejects results stamped with
  an older membership epoch.
- Two-party links first; three-party handoff later, if needed.

### 11.4 Knowledge replication

One source of truth per kind of data:

- **Mutable records** (memories, notes, preferences, tasks, contacts) are
  CRDT documents: data types that merge automatically. Notes use a text CRDT
  (so concurrent edits on two lamps both survive); single values use
  multi-value registers that show a conflict rather than silently dropping
  one side; sets are observed-remove sets. Every operation is signed by the
  lamp that wrote it and carries a hybrid-logical-clock timestamp. Every
  lamp can write while disconnected.
- **Bulk content** (files, model weights, embeddings, prebuilt indexes) is
  immutable, content-addressed blobs, fetched lazily with verified range
  reads. A file's "current version" is a multi-value pointer, so concurrent
  edits produce visible conflict copies.
- **Derived data** (search indexes, summaries) is rebuilt locally or shipped
  as immutable segments; it is never merged.
- **Security state is not ordinary replicated data.** Consent policy, intent
  permissions, broker configuration and membership live per lamp or in the
  membership log; a change made on another lamp takes effect only after
  confirmation on the local trusted surface.

Keys and names:

- Each private item has its own random data key, wrapped by its class key
  (knowledge, memories, secrets), which carries a key epoch; class keys are
  wrapped for each lamp allowed to hold that class. Each membership change
  starts a new epoch whose keys are generated by the entry's author and
  wrapped inside the entry; every ciphertext names its epoch; lamps keep a
  keyring of all epochs, and when log branches merge, the keys of both are
  kept.
- Private blobs are encrypted with their item's key and named by a keyed
  hash of the ciphertext; public blobs (model weights) keep plain BLAKE3
  names so they can be checked against published hashes.

Placement and durability:

- **Replication factors per class**: metadata, contacts and memories on every
  lamp; embeddings on every lamp in the PC ring; each file blob on at least
  two lamps. A lamp may drop a copy only after enough other lamps have given
  signed **holding commitments**, and a lamp that gave one keeps its copy
  until it hands the commitment on. Metadata is everywhere, so Jin can say
  "that file is on laptop 2" while offline.
- **Embeddings** come from one embedding model fixed for the whole mesh,
  versioned by its exact build (weights hash, quantization, tokenizer,
  chunker); on upgrade, content is re-embedded by version.

Forgetting ("Jin, forget X"):

1. It is its own intent: confirmed on the trusted path, never from a tainted
   session, with a cancellable delay (24–72 hours) before anything is
   destroyed.
2. Every wrapped copy of X's data key is deleted; a tombstone carrying only
   X's id (never a content hash) is written; CRDT state is compacted so old
   values can't be recovered; derived segments, key-value caches, traces and
   transcripts that include X are rebuilt or purged; journal entries about X
   become unreadable (section 13.6).
3. It is complete once every current member has acknowledged it; a removed
   or dormant lamp counts as complete, with a caveat shown to the owner.
4. After forgets (and periodically) class keys move to a new epoch and the
   remaining item keys are rewrapped, so stale wrapped copies left in flash
   become useless.

Absence:

- Forget tombstones (ids only) are kept forever; other tombstones are
  garbage-collected once every current member has acknowledged a version
  that includes them.
- A lamp absent longer than a set horizon (for example 60 days) is marked
  *dormant*, so garbage collection and forgets can proceed without it. When
  it returns, other lamps refuse its old operations; it receives current
  keys, rebuilds its replicated state, and packs its local-only changes
  into a quarantined bundle for the owner to review.

### 11.5 Using the mesh's compute

Measured results from projects that pool consumer devices (exo, prima.cpp,
distributed-llama, Petals) set the rules:

- **Route, don't split.** Each request runs whole on the best lamp that holds
  the needed model; tokens stream back. That costs one network round trip
  per request, not per token.
- **Parallel jobs are where the combined power shows**: indexing, embedding,
  transcription, summarizing new notes and files, several requests at once,
  each on its own lamp.
- **Splitting one model** is outside v1. Splitting by layers (pipeline) only
  adds capacity, not speed: prima.cpp ran a 70B model at about 0.7 s per
  token over home Wi-Fi on four devices that included two NVIDIA GPUs and an
  Apple M1, with disk offload; CPU-only laptops would be much slower.
  Splitting each layer (tensor parallelism) synchronizes many times per
  token: over Wi-Fi far too slow (about 150 ms per synchronization in
  prima.cpp's tests); over gigabit Ethernet it has sped up slow CPU nodes
  (distributed-llama: Llama 2 7B on Raspberry Pi 4s about 1.7× faster on 2
  nodes and 2.7× on 4, but 8 nodes were slower than 4). Measure on the two
  laptops before deciding.
- **Placement service**: a simple, explainable cost model first (queue time +
  prefill time + generation time at *sustained* speed + network cost,
  weighted by whose power is spent), using each lamp's measured profile;
  learned placement (section 8.1) only on top of it, with guards.
- **Failures**: if the lamp running a request disappears, the request
  restarts on the next lamp (processing the prompt again), and Jin tells the
  owner if the model changes. Only independent jobs are replayed.
- A sleeping laptop counts as unavailable for interactive work.

### 11.6 Time and partitions

- A lamp leaves the *unsynced* state (section 7.6) through NTP when online,
  or through the clock offsets measured to authenticated lamps during
  handshakes (the median of those it reaches); an unsynced lamp adopts that
  offset rather than rejecting its peers.
- A synced lamp quarantines remote clock values more than a set limit (for
  example 5 minutes) ahead of its own time, and alerts the owner.
- Certificates are checked with a stated tolerance; leases use local
  monotonic clocks only.
- Lamps keep working when cut off from each other; there is no quorum for
  ordinary operation. Data merges when they reconnect; side effects follow
  section 10.3.

### 11.7 Developing and testing the mesh

- `jin-meshd`'s core (the Noise state machine, remote-reference tables, the
  membership log, the CRDT store, forget tracking, placement) is written
  without I/O, with clock, randomness and transport injected. It builds into
  `jin-meshd` and into a **host simulator** that runs thousands of seeded
  scenarios per CI run: partitions, reordering, clock skew and jumps,
  crashes mid-write, lamps absent for months, concurrent key rotations and
  removals, mixed versions.
- Invariants checked: replicas converge after healing; no external effect
  happens twice; a removed lamp can't decrypt new data; forget either
  completes or reports why not; no edit made on either side of a partition
  is lost.
- Several QEMU lamps on a virtual network switch, from S5, for integration
  and chaos tests, with logged fault schedules (reproducible faults, not
  reproducible executions).
- QEMU lamps used for testing form their own **test mesh** with its own
  recovery key and synthetic data; the owner's mesh is the laptops plus at
  most one long-lived QEMU lamp.

### 11.8 Backup

Replication is not backup: deletes, forgets, bugs and a compromised lamp
all propagate. So:

- Versioned, encrypted, append-only snapshots go to a target outside the
  merge path (an external disk or the development PC) on a schedule.
- Class keys are also wrapped for the owner's recovery key, so the mesh can
  be restored after losing every lamp; device keys are never backed up (a
  restored lamp pairs as a new device), and the membership log is
  re-established with the recovery key.
- Item keys are kept in a small key store whose backups have a short, stated
  retention, so a forgotten item becomes unreadable in every backup once
  that window passes; "forget" shows as pending until then.

## 12. Running apps

### 12.1 Tiers

| Tier | What runs | How | When |
|---|---|---|---|
| Native | jinOS apps, Jin's surfaces | Native SDK, the compositor, intents | S12 onward |
| Compatibility VM | Unmodified Linux GUI apps: Firefox (Chromium second), Electron apps, office suites; WhatsApp Web | One Debian guest on VT-x with EPT; v1 shows the guest desktop as one window; per-app windows later (section 12.3) | S13 |
| Linux personality | The shell and command-line tools in v1 (D12); later the same GUI apps without a VM | Personality instances (D3) | S7 onward, growing for years |
| Android | Android apps (WhatsApp as the primary account; apps requiring certified devices excluded) | An Android system on the personality with binder (Starnix's approach), or an Android VM on laptops for x86 builds of apps; most Play apps are ARM-only and would need a proprietary ARM translator jinOS won't ship | After the phone port; not in v1 |

The VM tier is PC-only: phones are unlikely to offer usable hardware
virtualization (to be checked on the OnePlus 6), so the phone ring depends
on the personality running a browser, and on the Android tier for WhatsApp
as the primary account.

### 12.2 The compositor

**Wayland on Siraj.** Wayland normally runs over Unix sockets that pass file
descriptors. On jinOS its messages travel over Siraj channels, and each file
descriptor becomes a handle: shared-memory pools and keyboard maps become
memory objects (sealed against shrinking, so the compositor can map them
safely), and clipboard and drag-and-drop transfers use a byte-stream object.
The personality translates `AF_UNIX` and `SCM_RIGHTS` onto this transport,
and the VMM's bridge device plugs into the same one.

**Phase 1** (software only): the core protocol (including subsurfaces),
`xdg-shell`, `viewporter`, `fractional-scale`, `xdg-activation`,
`cursor-shape`, `xdg-decoration` (useful, not required), clipboard and
primary selection, `idle-inhibit`, XKB keyboard maps with layout switching
(for example English and Arabic), and `text-input-v3` with the compositor (or
a trusted service) as the input method. `presentation-time` waits for a GPU
driver, because the firmware framebuffer gives no vertical-blank signal.

**Scale**: wlroots implements these protocols in about 12,000 lines of C,
plus about 9,000 for its scene graph, output and cursor handling, on top of
libwayland-server (about 7,000 lines, which needs `epoll`, `signalfd`,
`timerfd` and descriptor passing) and xkbcommon. Budget 25–30k lines for
phase 1 written natively. Using wlroots instead needs a POSIX-like layer
under it (Q7).

**Performance**: the firmware framebuffer is mapped write-combining;
composition happens in a RAM back buffer and only damaged rectangles are
copied; there is no vertical sync, mode setting or external display until a
GPU driver exists; hidden windows get no frame callbacks; the compositor's
deadline class outranks inference and virtual CPUs.

**Text**: FreeType, HarfBuzz and a permissively licensed bidirectional-text
implementation render Arabic and other scripts correctly.

**The trusted path** (section 13.3): a secure attention key combination that
the compositor never forwards to any client; a reserved strip of the screen
no client can cover, even in fullscreen; a label on every VM and personality
window naming where it comes from; fullscreen for foreign clients only after
a user gesture, with a notice on how to leave. The trusted surface runs in a
small process separate from the Wayland protocol server, which is fuzzed.

Privileged protocols (screen capture, input injection, clipboard monitoring)
are offered only to clients that connected through an endpoint capability
allowing them. Jin never gets compositor-level input injection, screen
capture or overlays; brokered, confirmed app actions through the
accessibility bridge (section 10.4) are the only way it acts inside an app.

### 12.3 The compatibility VM

- **Guest**: a minimal Debian stable image with Firefox ESR, built
  reproducibly by jinOS's image builder from upstream Debian (nothing from
  `~/personal/mobile-os`), signed, with unattended security updates and a
  clock resync after the host resumes.
- **Display in v1**: the guest runs a single-window kiosk compositor (such as
  cage) on a virtio-gpu 2D framebuffer with virtio input; the VMM shows that
  framebuffer as one jinOS window. No Wayland bridge is needed for this.
- **Per-app windows after v1**: the VMM gains a Wayland bridge device
  (virtio-wl or the virtio-gpu cross-domain context, implemented natively in
  C++) and the guest runs Sommelier, which also brings X11 apps through
  XWayland. waypipe is not ported (it is GPLv3).
- **Memory**: one shared guest under a fixed RAM cap set once Q2 is known,
  memory faulted in on demand, virtio-balloon with free-page reporting, and
  the disk image opened without host caching so files aren't cached twice.
- **Network** (a capability): the host network service runs a user-mode
  proxy (a guest-facing network stack that terminates the guest's TCP and
  UDP and re-originates them through host sockets, plus a DNS forwarder).
  Internet egress by default; LAN access only if the owner grants it; never
  the mesh, the relay or host services. Bridging the guest onto wired
  Ethernet is optional.
- **Measured targets** in S13: scrolling frame rate, CPU use at a given video
  resolution, the highest video resolution that plays smoothly.

### 12.4 VM integration (after v1)

A guest agent over vsock, with each vsock port bound to one host capability:
launching guest apps from the jinOS launcher; forwarding guest notifications
to the host and to Jin, labelled untrusted (the cheapest route for Jin to
see WhatsApp messages); guest audio through virtio-snd; file sharing of
capability-scoped folders (Downloads, Shared) through virtio-9p, later
virtio-fs; and the AT-SPI bridge (section 10.4). The protocol is designed so
the personality can reuse it.

### 12.5 The browser

1. **v1**: Firefox in the compatibility VM, fully sandboxed, rendering in
   software. This is also the route to WhatsApp Web.
2. **Later**: unmodified Firefox and Chromium on the personality. The browser
   is the personality's acceptance test, especially the sandbox:
   - `seccomp` with `SECCOMP_RET_TRAP` delivering `SIGSYS` with exact
     `siginfo` and an editable `ucontext`, a classic-BPF interpreter in the
     personality (never inside the Linux process), and thread
     synchronization of filters;
   - `clone`/`clone3` with namespace flags, a zygote that forks without
     exec, `/proc/self/{maps,fd,status,uid_map,gid_map}` and `setgroups`;
   - `prctl` (`NO_NEW_PRIVS`, `DUMPABLE`), `rseq`, `eventfd`, `timerfd`,
     `inotify`, memory-file seals, `ptrace` for crash reporting, and fast
     TLB shootdown for JIT permission changes;
   - Chromium's and Firefox's own sandbox test suites as acceptance tests.
3. **Never**: no Chromium- or Gecko-based program (browsers, Electron, CEF
   apps) runs with its sandbox disabled; such apps stay in the VM until the
   personality passes the checklist. No downstream fork of a browser engine.
   Independent engines (Ladybird, Servo) are worth re-checking around 2028.

### 12.6 The Linux personality, step by step

- **Step 1** (S7): a static musl "hello world", then non-interactive BusyBox
  commands, with each Linux system call forwarded as an exception.
- **Step 2** (S7): interactive `sh` (terminal layer, job-control signals,
  pipes), page-level copy-on-write `fork`, a `/` tree mapped onto jinOS
  storage.
- **Step 3**: threads, the wait/wake call, thread-local storage, file `mmap`,
  time calls with a vDSO page; restricted mode replaces exception
  forwarding once measured.
- **Step 4a**: glibc programs, static and dynamic, with a complete auxiliary
  vector and `arch_prctl` working with restricted mode.
- **Step 4b**: `epoll`, sockets, `eventfd`/`timerfd`.
- **Step 5**: Wayland clients, then the browser checklist (section 12.5).

The file system first comes from a read-only image (EROFS or ext4) parsed by
the personality plus an in-memory layer, served without a round trip per
`stat` (WSL1's slowness came largely from translating each file operation
through a foreign I/O stack). Priorities come from data: system-call traces
(`strace -f`) of the target apps on ordinary Linux, ranked by use; anything
unimplemented returns `ENOSYS` and is counted. Conformance uses the project's
own tests and an LTP subset for steps 1–3, and gVisor's system-call tests
from step 4. Progress is tracked per target app (boots / renders / usable).

Scale, honestly: Google's Starnix is about 290,000 lines of hand-written
Rust (excluding generated bindings and tests) from a dedicated team, and
gVisor fully or partially implements 290 of the 352 x86-64 system calls in
its compatibility table (Linux now defines about 370). The personality is the
longest-running project in jinOS.

### 12.7 WhatsApp

- **On the laptops**: WhatsApp Web in Firefox in the VM, as a *linked
  device*. WhatsApp allows up to four linked devices; the account must live
  on the official app on a phone, and linked devices are logged out if that
  phone isn't opened for 14 days.
- **As the primary account on jinOS**: needs the Android tier, on a jinOS
  phone (or possibly an Android VM on a laptop, subject to WhatsApp's device
  and registration checks).
- **Not** unofficial WhatsApp clients or libraries: they break WhatsApp's
  terms and risk the owner's account being banned.
- Jin's access to WhatsApp content arrives after v1 (section 10.4); sending
  is always *confirm*.

### 12.8 What works when

| | v1 | Later | Never |
|---|---|---|---|
| Web browsing, web apps, WhatsApp Web | Firefox in the VM | Firefox/Chromium on the personality | |
| Linux GUI apps (GTK, Qt, Electron) | Inside the VM's single window | Per-app windows (Sommelier), then on the personality | |
| X11-only apps (Java, Wine, older Electron) | Inside the VM's window | Rootless via Sommelier | |
| Command-line Linux tools | BusyBox on the personality | glibc tools | |
| WebGL and GPU-heavy apps | No (Chromium has no WebGL without a GPU since M139; no GPU in the VM) | After a GPU driver | |
| Video calls, screen sharing from the VM | No | After camera, audio and capture support | |
| Guest audio | No | virtio-snd | |
| Printing, scanning, Bluetooth | No | Maybe | |
| Android apps | No | Android tier | ARM-only apps on x86 laptops; apps requiring a Google-certified device (banking, contactless payment, some streaming and games) |

## 13. Security and privacy

### 13.1 Threat model

**Assets**: the owner's memory and messages, account access, traces, keys,
device integrity, and the permissions granted to Jin.

**Attackers**:

1. The author of any content Jin reads: messages, web pages, documents, file
   names, audio played near a microphone. The most likely attacker.
2. A malicious or compromised app, including Linux programs and anything in
   the compatibility VM.
3. A compromised lamp in the mesh.
4. Someone with physical access to a lamp, locked or unlocked.
5. A nearby attacker: network or radio, a man-in-the-middle during pairing.
6. A thief with a powered-off lamp.
7. The supply chain: model files, weight blobs, packages, system updates,
   the development PC.

**Out of scope or only partly addressed**: a coercive partner who knows the
owner's passphrase (mitigations: no location history is kept, reading
private content aloud needs recent authentication); hardware implants;
attackers with physical access who can modify firmware.

### 13.2 Local authentication and storage at rest

- **Boot**: the owner's passphrase unlocks the lamp's storage key (Argon2id),
  which protects the disk and the device keys. A QEMU lamp in the test mesh
  may use a test passphrase.
- **Lock**: lamps lock after idle time; unlocking takes the passphrase (or a
  PIN set by the owner). The journal, memory views and anything that reads
  private content need an unlocked lamp.
- **After v1**: on UEFI laptops with TPM 2.0, optional TPM-plus-PIN sealing
  bound to measured boot (Limine can measure its configuration, Siraj and the
  initial RAM disk); where the firmware accepts custom Secure Boot keys,
  Limine signed with an owner key. BIOS-only or TPM 1.2 laptops keep the
  passphrase only. An unseal failure is always shown, never silently worked
  around.
- **DMA**: VT-d remapping is enabled for all devices at boot where present
  (section 7.5).
- Until encryption at rest exists (S6), a lost lamp's data is exposed; that
  is stated, not hidden.

### 13.3 Enforcement outside Jin

- **The broker**: Jin holds no raw capabilities for actions, only one handle
  to the broker. On every call, the broker checks the owner's policy, decides
  the consent level, shows any confirmation, invokes the implementing app and
  writes the journal.
- **Consent levels**: *never*, *confirm* (the default for anything new),
  *notify* (done, and Jin says so), *silent* (harmless things only).
- **Consent is keyed by (app, intent)**, never by intent name alone; a new
  app implementing an existing intent starts at *confirm*. An intent counts
  as **off-device** if the implementing app holds, or later gains, any
  capability that reaches a network or another person (sends, drafts that
  sync to a server, invites, opening or fetching a URL Jin composed, shares);
  off-device intents are never *silent*, and the classification is re-checked
  when the app's grants change.
- **Policy is out of Jin's reach**: consent levels, allow-lists, membership,
  grants, Reflex switches, journal retention and the active model are not
  reachable through any intent; the owner changes them only in settings on
  the trusted path.
- **The trusted path** (section 12.2): confirming means pressing the secure
  attention key, then acting on the broker's surface; input is ignored for
  about half a second after the surface appears, and queued input is
  discarded. The surface shows the exact arguments (recipient, full text,
  attachments), the requesting lamp and the request's origin; it renders
  bidirectional-text controls and invisible characters visibly, isolates
  each argument's direction, and flags mixed-script or look-alike recipient
  names. A spoken "yes" is never a confirmation.
- **Across lamps**: a proposal computed elsewhere is confirmed on the lamp
  where the owner is. Confirming produces a **confirmation record** signed
  with that lamp's device key: a hash of the exact arguments, the idempotency
  key, the taint label, the executor and a short expiry. A broker on another
  lamp acts only on such a record, at most once per key, only if the
  arguments match exactly, and under its own policy for remote requests
  (which lamps may request which intents, how often, and which intents always
  need local confirmation). It journals and shows a notice of every remotely
  confirmed effect. A compromised member lamp can still issue such records;
  these limits and the notices, not the signature, are the defence.
- Jin's own network access is limited to configured services; its interface
  never loads remote images or link previews automatically. Model downloads
  go through a separate fetch service (section 13.9).
- Jin may *suggest* things unprompted, but acts unprompted only on intents
  the owner set to *notify* or *silent*.

### 13.4 Untrusted content

- Jin can read untrusted sources only through capabilities the broker
  issues, so the broker knows when a session has read one. Voice requests
  captured while a lamp is playing audio, and anything from the VM
  (notifications, clipboard, accessibility data), are untrusted.
- Until precise tracking exists, the rule is coarse: once a session has read
  any untrusted source, every off-device effect and every destructive
  on-device intent (delete, overwrite, settings, memory edits, forget) in that
  session is *confirm*, whatever the owner's setting. Only the owner starts a
  fresh session, through the trusted path.
- **Taint is a label on data.** Memories derived from untrusted sources keep
  an "untrusted" label permanently; the label is a grow-only set merged by
  union, so no merge or edit can drop it; a derived item's label is the
  union of its inputs'; lowering a label takes an explicit owner action on
  the trusted surface. Everything a tainted session creates (notes,
  reminders, scheduled jobs, files) inherits the taint; tasks handed to
  another lamp carry their labels, which the receiver can raise but never
  lower. When a tainted value decides a recipient, URL or file target, the
  confirmation says so.
- The target for precise tracking: a privileged planner that sees only the
  owner's request and trusted data, a quarantined model with no tool access
  that extracts values from untrusted content, and per-argument provenance
  in the broker (the CaMeL design, Debenedetti et al., 2025).

### 13.5 Who is speaking

Voice (after v1) is a convenience, never proof of identity. Each request
carries its origin (typed on an unlocked console, voice, or another lamp) and
the capturing lamp's lock state, and the lock rules follow it through
handoff. While locked, Jin runs only silent intents that reveal nothing and
never reads memory, messages or the journal aloud.

### 13.6 The journal and recovery

- The **broker**, not Jin, writes the journal into an append-only object Jin
  can't modify. Entries are hash-chained and record the intent, the consent
  level, the taint label, the capability used, and the arguments, encrypted
  under the data key of the item they concern, so forgetting that item makes
  them unreadable while the chain (computed over ciphertext) still verifies.
- Every few minutes the broker signs the journal head and replicates heads
  and entries to the other lamps, which act as witnesses; truncation writes
  a signed checkpoint first. Entries for one action on different lamps share
  its idempotency key, so the journals join into a mesh-wide view.
- **Recovery by construction**:
  - `file.delete` moves files to a trash Jin can't empty (30 days), and the
    trash replicates with the deletion; when space runs out, Jin's deletes
    are refused rather than older trash purged;
  - files Jin writes or overwrites keep their previous version for 30 days;
  - Jin's changes to replicated records are reversible operations that carry
    the previous value;
  - notify-level sends wait through an undo window of a few seconds;
  - irreversible intents (forget, accessibility actions, sends past their
    undo window) are listed and are always *confirm*.

### 13.7 Reflexes and traces

- **Bounded harm** (section 8.2, rule 5). Per-app caps (preload size, cache
  residency) limit what any app can gain by gaming the training data; the
  watchdog checks per-app regressions; scheduler and memory policies never
  demote the compositor, the trusted surface or the broker.
- **Traces**: readable only by the trainer; raw traces kept for a limited
  number of days, after which only trained weights and aggregates remain;
  location, if ever used, is a coarse label (home / work / other), never
  coordinates; the owner can exclude apps from recording; exports to the
  development machine are confirmed and redacted.
- **Learned state isn't observable by other apps**: residency queries report
  only on the caller's own memory, and model objects are mapped only into the
  inference process. For policies whose effects an app can observe through its
  own refaults or latency, features come from that app or file only, or the
  cross-app leak is documented.
- The trainer's trace parser is fuzzed; the trainer has no network.

### 13.8 Foreign software

- Each personality container gets explicit handles only, one personality
  instance per container, and no capabilities to Jin's stores or keys. The
  personality is part of the trusted computing base for the programs it
  hosts.
- The compatibility VM is one protection domain (D13): per-app policy
  applies to the VM as a whole. Separate VMs per trust group (for example a
  browser VM and an everything-else VM) are possible at extra memory cost.
- Clipboard data from a VM counts as untrusted and crosses only on an
  explicit paste by the owner.

### 13.9 Updates and supply chain

- **System images** (Siraj, the broker, `jin-meshd`, the compositor) are
  signed with a release key on a hardware token, separate from mesh keys and
  never stored on the development PC's disk. A signed manifest carries a
  version that only increases; lamps refuse older versions.
- Updates travel over the mesh as content-addressed blobs, but only the
  release signature authorizes them; no lamp can push one. The owner approves
  installation on the trusted path, one lamp first.
- The previous system image stays selectable in the boot menu; A/B system
  partitions with automatic rollback come after v1.
- The VM guest updates itself from Debian's signed repositories, and its
  image is rebuilt on a schedule.
- Vendored dependencies are pinned by hash and watched for security
  advisories.
- **Models** are fetched by a separate service against an owner-signed
  manifest of pinned hashes and sources; Jin asks for a model by hash and
  can't fetch anything else. Prompt templates are fixed per model family in
  jinOS; templates embedded in model files are never executed. Changing the
  active model is a policy change on the trusted path. Model-file parsers are
  fuzzed.

### 13.10 The mesh

- Pairing is owner-verified (section 11.1); each lamp has its own keys; a
  handoff carries a proposal and its taint label, never capabilities.
- Every remote job carries only the context it needs; what context may leave
  a lamp is a per-class policy.
- Discovery and unauthenticated input are tiny, fixed-format and
  resource-capped before authentication. HarmonyOS's equivalent daemon
  shipped radio-reachable vulnerabilities, including an authentication
  bypass, so the authentication state machine is explicit and every message
  is checked against it.
- **Losing a lamp**: the owner removes it from another lamp (or with the
  recovery key), which starts a new key epoch; its certificate expires; data
  already on it is protected by its encryption at rest. Revocation also shows
  a checklist of external sessions to end (WhatsApp → Linked devices → log
  out, browser sessions, service tokens) and rotates the secrets class.
- Operations a removed lamp wrote are attributed (they are signed), so the
  owner can quarantine everything it wrote after a chosen time.

## 14. Language, toolchain and repository

### 14.1 Toolchain and reused code

- **C++20** for all new code, with the kernel rules in section 7.8. Python
  for training, trace tools and build helpers.
- **Toolchain**: Clang/LLVM and LLD, CMake with Ninja (a superbuild keeping
  host, kernel and user builds separate), QEMU, GDB, Bochs, Limine. Clang,
  LLD, CMake and Ninja need installing on the development PC first.
- **Reused code** (licences to confirm against D10):
  - Boot and platform: Limine (BSD), uACPI (MIT), Intel microcode.
  - User space: musl (MIT), libc++/libc++abi/libunwind (Apache-2.0 with LLVM
    exception), lwIP (BSD).
  - Cryptography: libsodium (ISC) or Monocypher (BSD-2-Clause/CC0), the
    BLAKE3 reference code, an existing SPAKE2+ or CPace implementation.
  - Text and input: FreeType (FTL), HarfBuzz (Old MIT), libxkbcommon (MIT),
    xkeyboard-config, and a permissive bidirectional-text implementation
    (ICU's or SheenBidi, Apache-2.0); not FriBidi (LGPL).
  - AI: ggml/llama.cpp (MIT).
  - Optional: wlroots (MIT), only with a POSIX-like layer (Q7).

### 14.2 The native SDK

Native jinOS programs (`jin-meshd`, netd, the compositor, the VMM, the
broker, the Mind) need a user-space runtime:

- a jinOS target for Clang and a sysroot;
- musl ported to Siraj's system calls (Fuchsia's libc is also derived from
  musl);
- libc++, libc++abi and libunwind; exceptions allowed in user space (D8);
- threads on Siraj threads, the wait/wake call, thread-local storage through
  the FS base register;
- static linking first;
- an event loop over ports and channels.

Minimal in S2, complete in S4.

### 14.3 Repository

```
jinos/
  docs/            this document, decisions, memory layout, experiment reports
  kernel/
    arch/x86_64/   boot entry, interrupts, paging, hardening, context switch, VMX
    core/          objects, handles, scheduler, memory, IPC, time, random
    fs/            page cache, block layer, file system
    reflex/        Reflex library, event recorder, weight checker
    model/         model objects
    drivers/       virtio, AHCI, NVMe, xHCI, i8042, e1000e/r8169, ...
  lib/kstd/        kernel support library
  sdk/             native sysroot, musl port, runtime
  user/
    init/          first process
    broker/        intent broker, trusted surface, journal
    meshd/         jin-meshd (sans-I/O core + I/O shell)
    netd/          network stack and VM network proxy
    personality/   Linux personality
    vmm/           virtual machine monitor
    compositor/    Wayland compositor
    jin/           the assistant, the Mind, the planner
    services/      notes, files, reminders, backup, model fetch, placement, trainer
  tools/           trace simulator, mesh simulator, weight packer, image builders, QEMU cluster runner
  train/           PyTorch training and quantization
  tests/
```

## 15. Roadmap

### 15.1 The first two weeks

1. Choose the licence (Q1).
2. Install the toolchain (Clang, LLD, CMake, Ninja, QEMU, Limine, GDB).
3. Create the repository skeleton: README, this document,
   `docs/memory-layout.md`.
4. Collect the laptops' details (Q2), to fix the first real drivers.
5. S0: Siraj boots through Limine in QEMU, prints over serial and draws on
   the framebuffer; CI does the same on every change.
6. Start the trace lab (section 8.6) on the development PC in the background.

### 15.2 Milestones

| Milestone | Work | Done when |
|---|---|---|
| S0 Foundations | Repository, Clang cross build with CMake, Limine image, QEMU runner, GDB script, CI boot test, framebuffer panic screen, kstd basics | `ninja run` boots and prints over serial and on screen; CI boots on every commit |
| S1 Core | Interrupts (APIC, MSI), timers and TSC calibration, real-time clock read, physical allocator, slab heap, own page tables and direct map, SMP bring-up, kernel threads, preemptive per-CPU scheduler, hardening baseline (7.2), random-number generator, ACPI tables, debug-build checkers | Threads on several CPUs pass TLB-shootdown and IPI torture tests and a locked-counter stress test |
| S2 User space | Objects and handles, address spaces, demand-zero faults, ring 3, system calls, user-copy API, KPTI where needed, ELF loader, initial RAM disk, console, saved FPU state, wait/wake, `getrandom`; minimal native SDK | A native program runs, and a second receives a handle from the first |
| S3 Laptop smoke test | Boot the S2 image on both laptops from USB and over the network | All CPUs come up, the framebuffer log and keyboard work, and everything that breaks is written down |
| S4 Native SDK | Sysroot, musl port, libc++, threads, TLS, event loop, crypto library | `jin-meshd`'s core and its unit tests build and run natively |
| S5 Two lamps | virtio-net, netd (lwIP), `jin-meshd` v0 over Noise/UDP with ephemeral test keys, a remote capability, the host mesh simulator, the QEMU cluster runner | Two QEMU lamps exchange authenticated messages; a broken link reports as broken; the simulator runs seeded fault scenarios in CI |
| S6 Storage | Block layer with an async queue and flush, page cache, file-backed faults, file system (Q5), `fsync` and atomic rename, checksummed append-only record files, encryption at rest | Files persist; a file larger than RAM reads through `mmap`; after QEMU is killed at random points, all `fsync`ed data is intact |
| S7 Linux shell | Personality steps 1–2 (12.6) | BusyBox `sh` runs interactively with pipes over a `/` mapped onto jinOS storage |
| S8 Real laptop | Install to disk, network-boot development loop, network console, persistent panic log; AHCI or NVMe, keyboard, Ethernet, real-time clock, ACPI basics, lock and unlock | Laptop 1 boots from its disk to BusyBox `sh` and joins a QEMU lamp over Ethernet |
| S9 Mesh v1 | Membership log, PAKE pairing, device certificates, key epochs, signed CRDT store with text CRDT, blob store with holding commitments, forget, time sync, versioning, revocation, backup; laptop 2 joins once its drivers exist | Laptops and a QEMU lamp share notes; every edit made on both sides of a partition survives the merge; a removed lamp is cut off; after wiping every lamp, notes restore from backup; the simulator's invariants hold, including mixed versions |
| S10 Reflex infrastructure | Event recorder, Reflex library in the kernel, weight checker, shadow mode, ghost lists, randomized control, watchdog, epoch-based swap | Replayed traces give bit-identical decisions in kernel and simulator; the weight checker survives fuzzing; a deliberately bad weight set is caught |
| S11 Hypervisor | VMX with EPT, split irqchip, minimal VMM, Linux guest by direct kernel boot | A Debian guest boots to a shell over virtio-console, with block and vsock, under nested KVM on the development PC and on a laptop |
| S12 Compositor | Wayland on Siraj, phase-1 protocols, fonts and keymaps, touchpad and backlight, the trusted path | A native client shows a window on a laptop; the secure attention key reaches only the broker's surface |
| S13 Browser | virtio-gpu 2D and input, the guest desktop as one window, VM networking | Firefox runs in a window on a laptop and WhatsApp Web works; the measured targets in 12.3 are recorded |
| S14 Jin on one lamp | Native ggml with a user-space expert cache; model loader and fetch service; broker, consent, trusted confirmation, journal; `note.create`, `file.find`, `reminder.set`; vCard and iCalendar import; intent model; saved prompt prefix | By text, the owner asks Jin on a laptop; it answers from its memory of the owner and creates a confirmed reminder |
| S15 Jin across lamps | Placement, confirmation records, executors, journal witnesses | A conversation started on laptop 1 continues on laptop 2; across an injected partition a confirmed action happens at most once and is never silently repeated; forget completes on every member |
| S16 First in-kernel Reflex | After at least four weeks of daily use, measure headroom on Siraj's own traces (model-weight readahead or the model-object cache versus the user-space cache; the VM's disk-image readahead); train and evaluate per 8.5 | Evaluated on Siraj traces; live only if it beats its fallback, otherwise the result is written up and the fallback ships |
| After v1 | VM integration (12.4); audio and voice; AT-SPI bridge; per-app VM windows; personality steps 3–5; Wi-Fi; battery and lid; the relay; QR pairing; A/B updates; TPM sealing and measured boot; then the next rings: phones (ARM64 port), the Android tier, GPUs | |

### 15.3 Releases and v1

- **jinOS 0.5, "the mesh"** (after S9): Siraj on two laptops and a QEMU
  lamp as one authenticated mesh with replicated, backed-up notes and a
  Linux shell.
- **jinOS v1** (after S16): Jin by text on either laptop, with shared memory
  of the owner, three intents through the broker with consent, trusted
  confirmation and journal, and requests placed on the best lamp; Firefox and
  WhatsApp Web in the compatibility VM; the Reflex infrastructure complete
  and at least one in-kernel Reflex evaluated on Siraj's own traces. Jin
  cannot read or send WhatsApp messages in v1.

| Vision item (section 2) | Delivered by |
|---|---|
| 1. One Jin, every lamp | S9 (shared memory), S15 (continuing conversations) |
| 2. The lamps work as one | S15 (placing requests); parallel background jobs after v1 |
| 3. Acts, safely | S12 (trusted path), S14 (broker, consent, journal) |
| 4. Private by construction | S6 (encryption at rest), S9 (keys), S14 (local models) |
| 5. Gets better at being yours | S10, S16 (Reflexes); on-device training after v1 |
| 6. Honest about what it did | S10 (Reflex tracing), S14–S15 (journal) |

### 15.4 Effort

In focused full-time months, for a first-time kernel author:

| Milestones | Estimate |
|---|---|
| S0–S3 | 6–9 |
| S4–S7 | 8–14 |
| S8–S9 | 6–10 |
| S10–S13 | 10–16 |
| S14–S16 | 6–10, plus at least four weeks of calendar time for traces |
| **Total to v1** | **about 3–5 years** (jinOS 0.5, after S9, at about 20–33 months) |

These are judgment, not measurement: SerenityOS's kernel had one experienced
full-time author; Zircon and Starnix were built by teams. First-kernel
estimates often run long. Record actual against estimated time per
milestone, and re-plan after S1 and again after S2. Estimates in
full-time months rescale if work becomes part-time.

### 15.5 Installing, updating and recovering

- **Install**: the laptop either gives its whole disk to jinOS or dual-boots
  (Q9). Until v1, jinOS never holds the only copy of the owner's data
  (principle 10).
- **Development loop**: laptops boot new builds over the network from the
  development PC (Limine's PXE support), with the network console and the
  persistent panic log for debugging.
- **Updates**: signed images (section 13.9); the previous image stays in the
  boot menu; one lamp first.
- **Recovery**: from the previous image in the boot menu; from backup
  (section 11.8); after losing every lamp, with the recovery key.

## 16. Risks

| Risk | Mitigation |
|---|---|
| Scope: kernel, mesh, AI, compatibility and UI for one person | Small, complete milestones; an intermediate release (0.5); reuse; re-plan after S1 and S2; a smaller v1 if needed (Q10) |
| First-kernel estimates run long | Track actuals; cut the scope of a step rather than its tests |
| Laptop hardware unknown or old (no EPT, no AVX2, no TPM, no Ethernet port, BIOS only) | Collect specs first (Q2); Limine handles BIOS; a USB Ethernet adapter; ggml chosen per CPU; passphrase-based keys; a laptop without VT-x with EPT serves as a compute and storage lamp without Linux GUI apps until the personality matures |
| CPU vulnerabilities on old Intel laptops | The hardening baseline (7.2), microcode loading, KPTI where needed |
| Laptop drivers (Wi-Fi, GPU, sleep, audio) | Ethernet first; software rendering; Wi-Fi via a driver VM where VT-d exists; all after v1 |
| In-kernel Reflexes may not beat strong baselines | Headroom measured first; bounded actions; fallbacks always on; a negative result is acceptable and written up |
| The mesh: partitions, clock drift, compromised lamps | Explicit remote references, signed and attributed operations, causal membership log with vetoes, time rules, the host simulator from S5 |
| Prompt injection makes Jin act against its owner | The broker outside Jin, trusted path, taint labels, consent by (app, intent), journal with witnesses |
| Spoofed confirmations | Secure attention key, reserved screen strip, window labels, a separate trusted-surface process |
| The Linux personality and the VMM are large attack surfaces | One personality instance per container; one VMM process per VM; fuzzing; the VM as one protection domain with no access to Jin's data |
| Data loss on an experimental OS | Principle 10, backups outside the mesh, crash-safe record files, power-cut tests |
| Kernel memory-safety bugs in C++ | kstd RAII types, sanitizers and checkers in debug builds, fuzzing |
| Models too slow on old laptops | Small models in RAM, the tiny intent model for common commands, heavy requests to the strongest lamp |

## 17. Open questions

- **Q1. Licence.** A permissive licence (BSD-2-Clause or Apache-2.0) fits a
  standalone system and the reused libraries; Linux drivers, if ever reused,
  would stay in a separate program (the driver VM). GPL-2.0 would allow
  porting Linux driver code into Siraj, and BSD drivers are a permissive
  porting source either way. Decide before the first commit.
- **Q2. The laptops.** Model and year; firmware (UEFI or BIOS only; whether
  custom Secure Boot keys can be enrolled); CPU model and stepping, flags
  (VT-x, EPT, unrestricted guest, VT-d, AVX, AVX2, FMA, XSAVE, x2APIC, PCID,
  RDRAND/RDSEED) and its listed vulnerabilities (from
  `/sys/devices/system/cpu/vulnerabilities/` under a Linux live USB); RAM;
  disk (HDD, SATA SSD or NVMe); Ethernet port and chip; USB controller
  (EHCI or xHCI); touchpad bus (PS/2, I2C or USB); TPM (none, 1.2, 2.0,
  firmware); Intel AMT; Wi-Fi chip.
- **Q3. Wi-Fi.** A driver VM (needs VT-d) or a native driver for the
  specific chip, and when.
- **Q4. Compositor.** From scratch in C++ (about 25–30k lines for phase 1),
  or wlroots (needs a POSIX-like layer, see Q7).
- **Q5. File system.** ext2 (built and checked with standard tools; repaired
  to consistency after a crash, with critical data in its own crash-safe
  records) or a small journaled or log-structured format of our own.
- **Q6. Mesh RPC and CRDTs.** Own protocol and CRDT types in C++, or Cap'n
  Proto's C++ implementation and an existing text-CRDT design ported to C++.
- **Q7. A POSIX shim for native programs** (an `epoll`/`timerfd`-style layer
  on the native SDK): it would allow wlroots and other reused code, at a cost.
- **Q8. Recovery key format.** Paper, split shares or a hardware key.
- **Q9. Install.** Wipe the laptops or dual-boot.
- **Q10. A smaller v1?** If 3–5 years is too long: a "mesh and Jin" v1
  without the compatibility VM and the compositor (Jin by text in the
  console, with confirmations on a console-based trusted surface; the owner
  keeps another OS for browsing) would be about S0–S10 plus S14–S16,
  roughly 2.5–4 years.
- **Q11. Next ring.** When to start the ARM64 phone port, on which phone,
  and checking that phone's virtualization support.
- **Q12. Names.** Siraj, Lamp, Reflex, Mind: keep or change.

## 18. Glossary

**Kernel basics**

- **Higher-half kernel**: the kernel lives at the top of every address
  space. **Direct map**: a region where all RAM appears at fixed addresses.
- **Long mode**: the CPU's 64-bit mode. **Ring 3**: user mode.
- **TLB shootdown**: telling other CPUs to forget cached address
  translations after a mapping changes.
- **APIC / x2APIC / I/O APIC**: the interrupt controllers in each CPU and on
  the chipset. **TSC / HPET**: the CPU's cycle counter and a chipset timer.
- **MCFG**: the ACPI table locating PCIe configuration space. **MSI**:
  interrupts delivered as memory writes.
- **Slab allocator**: a memory allocator for many objects of one size.
- **Demand paging**: memory filled in only when first touched.
  **Copy-on-write**: shared pages copied only when one side writes.
- **MMIO / DMA / IOMMU**: device registers mapped as memory; devices reading
  and writing RAM themselves; hardware that restricts which RAM a device may
  touch (Intel's is VT-d).
- **vDSO**: a small piece of kernel-provided code mapped into processes, so
  they can read the time without a system call.
- **Futex / wait-wake**: a system call to sleep until a memory location
  changes.
- **EROFS**: a compact read-only file system. **vsock**: a socket between a
  VM and its host. **binder**: Android's IPC mechanism. **LTP**: the Linux
  Test Project.
- **KPTI, SMEP, SMAP, retpoline**: defences against CPU side channels and
  against the kernel being tricked into using user memory.
- **Trusted computing base**: the parts that must be correct for security to
  hold.

**jinOS terms**

- **Capability / handle**: a token that both names something and grants
  specific rights to it; without it a program can't even refer to the thing.
- **Ambient authority**: power a program has just by existing (such as
  opening any file by path); jinOS avoids it.
- **Caretaker**: a forwarding object whose owner can destroy it to revoke
  everything granted through it.
- **Badge**: a value fixed into a handle by its minter, attached by the
  kernel to every message sent through it.
- **Personality**: a layer implementing another OS's system calls so its
  programs run unmodified. **Restricted mode**: the kernel feature that
  sends a foreign program's system calls straight to its personality.
- **Split irqchip**: the guest's per-CPU interrupt controller emulated in the
  kernel, the rest in the VMM.
- **Page cache**: file contents kept in RAM. **Refault**: a page evicted and
  needed again soon. **Readahead**: reading the next part of a file before
  it is asked for. **Belady's optimal**: the best possible eviction, knowing
  the future; used to measure headroom. **CLOCK-Pro / multi-generation LRU**:
  strong non-learned eviction rules.
- **Quantization**: storing model numbers in fewer bits. **Prefill**:
  processing the prompt before the first output token. **MoE**: a model that
  activates only some of its weights per token.
- **Shadow mode**: running a policy's predictions without acting on them, to
  score them. **Ghost list**: a record of what a policy would have evicted.
- **CRDT**: a data type whose separately edited copies always merge
  automatically. **Hybrid logical clock**: a timestamp combining wall-clock
  time with a counter. **Tombstone**: a marker that something was deleted.
- **Idempotency key**: a unique ID on an action, so a repeat can be
  recognized. **Executor**: the one lamp allowed to perform an action.
- **Content-addressed**: named by a hash of the content. **Crypto-shredding**:
  deleting data by destroying the only key that decrypts it. **Key epoch**:
  one generation of keys, replaced when membership changes.
- **Noise protocol**: a framework for authenticated, encrypted handshakes,
  used by WireGuard. **PAKE**: a key exchange that turns a short shared code
  into a strong key. **DNS-SD**: service discovery on a local network. **NAT
  traversal**: connecting two machines behind home routers.
- **Trusted path / secure attention key**: a way for the owner to reach the
  system that no app can imitate or intercept.
- **VT-x / EPT**: Intel's hardware support for virtual machines and for their
  memory.
- **Wayland**: the protocol Linux applications use to show windows.
  **AT-SPI**: the Linux accessibility interface through which apps describe
  their user interface.

## 19. References

Kernels and compatibility:
- Fuchsia: Zircon objects and handles, hypervisor and VMM; RFC-0082
  (Starnix), RFC-0261 (restricted mode). https://fuchsia.dev
- seL4 (capabilities, badges). https://sel4.systems
- gVisor system-call compatibility tables.
  https://gvisor.dev/docs/user_guide/compatibility/
- Microsoft, "Comparing WSL 1 and WSL 2".
  https://learn.microsoft.com/windows/wsl/compare-versions
- ChromeOS Crostini; crosvm; Sommelier. Firecracker.
- Limine; uACPI; SerenityOS; xv6; the OSDev wiki (https://wiki.osdev.org).

Distributed systems:
- Waldo et al., "A Note on Distributed Computing" (1994).
- Cap'n Proto RPC; the E language; OCapN.
- Ink & Switch, "Local-first software" (2019); Automerge; Syncthing.
- Tailscale, Tailnet Lock; the Noise Protocol Framework; WireGuard.
- OpenHarmony distributed soft bus and data management.
- RFC 9383 (SPAKE2+); CPace.

AI systems and security:
- Hashemi et al., "Learning Memory Access Patterns" (ICML 2018).
- Hao et al., LinnOS (OSDI 2020); Akgun et al., KML (in-kernel ML, readahead).
- Alizadeh et al., "LLM in a Flash" (Apple, 2023); Song et al., PowerInfer
  (2023); TEAL (training-free activation sparsity, ICLR 2025).
- prima.cpp (2025); distributed-llama; exo; Petals; llama.cpp / ggml.
- Debenedetti et al., CaMeL (2025): defeating prompt injection by design.
- MAUI, CloneCloud (mobile offloading).
