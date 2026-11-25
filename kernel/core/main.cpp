// Siraj entry point. S0: come up, say what we found, stop.

#include "arch/x86_64/cpu.h"
#include "arch/x86_64/serial.h"
#include "core/boot_info.h"
#include "core/fbcon.h"
#include "core/panic.h"
#include "core/print.h"
#include "kstd/string.h"

#define SIRAJ_VERSION "0.0.1-s0"

namespace {

constinit boot::BootInfo g_boot;
constinit bool g_selftest = false;

// QEMU's isa-debug-exit device: writing v to port 0xf4 exits QEMU with
// status (v << 1) | 1. Harmless on real hardware (nothing listens there).
constexpr u16 QemuExitPort = 0xf4;
constexpr u8 QemuExitOk = 0x10;      // exit status 33
constexpr u8 QemuExitFailed = 0x11;  // exit status 35

void qemu_exit_hook(bool failed) {
    if (!g_selftest) return;
    arch::outb(QemuExitPort, failed ? QemuExitFailed : QemuExitOk);
}

void serial_sink(const char* s, usize n) { arch::serial::write(s, n); }
void fbcon_sink(const char* s, usize n) { fbcon::write(s, n); }

void log_cpu() {
    char vendor[13] = {};
    auto id = arch::cpuid(0);
    memcpy(vendor + 0, &id.ebx, 4);
    memcpy(vendor + 4, &id.edx, 4);
    memcpy(vendor + 8, &id.ecx, 4);

    char brand[49] = {};
    if (arch::cpuid(0x80000000).eax >= 0x80000004) {
        for (u32 i = 0; i < 3; i++) {
            auto r = arch::cpuid(0x80000002 + i);
            memcpy(brand + i * 16 + 0, &r.eax, 4);
            memcpy(brand + i * 16 + 4, &r.ebx, 4);
            memcpy(brand + i * 16 + 8, &r.ecx, 4);
            memcpy(brand + i * 16 + 12, &r.edx, 4);
        }
    }
    const char* b = brand;
    while (*b == ' ') b++;
    klog("cpu", "%s, %s", vendor, *b ? b : "(no brand string)");

    auto f1 = arch::cpuid(1);
    auto f7 = arch::cpuid(7);
    klog("cpu", "features: %s%s%s%s%s%s%s%s",
         (f1.ecx & (1u << 5)) ? "vmx " : "",
         (f1.ecx & (1u << 21)) ? "x2apic " : "",
         (f1.ecx & (1u << 26)) ? "xsave " : "",
         (f1.ecx & (1u << 28)) ? "avx " : "",
         (f7.ebx & (1u << 5)) ? "avx2 " : "",
         (f1.ecx & (1u << 17)) ? "pcid " : "",
         (f7.ebx & (1u << 7)) ? "smep " : "",
         (f7.ebx & (1u << 20)) ? "smap " : "");
}

void log_memory_map() {
    u64 usable = 0;
    klog("mem", "memory map (%zu regions):", g_boot.memory_count);
    for (const auto& r : g_boot.memory_map()) {
        print::printf("  %016lx-%016lx  %10lu KiB  %s\n", r.base.value, r.base.value + r.length - 1,
                      r.length / kstd::KiB, boot::memory_type_name(r.type));
        if (r.type == boot::MemoryType::Usable) usable += r.length;
    }
    klog("mem", "%lu MiB usable", usable / kstd::MiB);
}

}  // namespace

extern "C" [[noreturn]] void kmain() {
    arch::serial::init();
    print::add_sink({serial_sink});

    if (!arch::collect_boot_info(g_boot)) {
        print::puts("siraj: boot loader protocol too old (need Limine base revision 4+)\n");
        arch::halt_forever();
    }
    g_selftest = kstd::has_word(g_boot.cmdline, "selftest");
    panic_hooks::set_exit_hook(qemu_exit_hook);

    if (fbcon::init(g_boot.framebuffer)) print::add_sink({fbcon_sink});

    print::printf("\nSiraj " SIRAJ_VERSION " (jinOS)\n\n");
    klog("boot", "%s %s, protocol revision %lu, %s firmware", g_boot.loader_name, g_boot.loader_version,
         g_boot.protocol_revision, g_boot.uefi ? "UEFI" : "BIOS");
    klog("boot", "cmdline: \"%s\"", g_boot.cmdline);
    klog("boot", "kernel at phys %#lx, virt %#lx; direct map at %#lx", g_boot.kernel_phys_base.value,
         g_boot.kernel_virt_base.value, g_boot.hhdm_offset);
    klog("boot", "serial %s, RSDP %#lx, boot time %lu", arch::serial::present() ? "present" : "absent",
         g_boot.rsdp.value, g_boot.boot_time_unix);
    if (g_boot.framebuffer.present()) {
        const auto& fb = g_boot.framebuffer;
        klog("boot", "framebuffer %ux%u, %u bpp, pitch %u, %ux%u text", fb.width, fb.height, fb.bpp, fb.pitch,
             fbcon::columns(), fbcon::rows());
    } else {
        klog("boot", "no framebuffer");
    }
    for (usize i = 0; i < g_boot.module_count; i++) {
        klog("boot", "module %zu: %s (%zu bytes)", i, g_boot.modules[i].path, g_boot.modules[i].size);
    }
    log_cpu();
    log_memory_map();

    if (kstd::has_word(g_boot.cmdline, "panic")) panic("requested on the command line");

    klog("boot", "S0 ok");
    if (g_selftest) qemu_exit_hook(false);
    arch::halt_forever();
}
