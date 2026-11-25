// What the boot loader hands us, in the kernel's own terms. Filled in by
// arch code from the Limine responses; nothing else in the kernel reads
// Limine structures directly.
#pragma once

#include "kstd/span.h"
#include "kstd/types.h"

namespace boot {

enum class MemoryType : u8 {
    Usable,
    Reserved,
    ReservedMapped,       // reserved, but mapped in the direct map (firmware tables, EFI runtime)
    AcpiReclaimable,
    AcpiNvs,
    Bad,
    BootloaderReclaimable,
    KernelAndModules,
    Framebuffer,
};

struct MemoryRegion {
    kstd::PhysAddr base;
    u64 length = 0;
    MemoryType type = MemoryType::Reserved;
};

struct Framebuffer {
    u8* address = nullptr;        // virtual address, write-combining mapping
    u32 width = 0, height = 0;    // pixels
    u32 pitch = 0;                // bytes per row
    u16 bpp = 0;
    u8 red_shift = 0, green_shift = 0, blue_shift = 0;
    u8 red_size = 0, green_size = 0, blue_size = 0;
    bool present() const { return address != nullptr; }
};

struct Module {
    const void* address = nullptr;
    usize size = 0;
    const char* path = nullptr;
    const char* string = nullptr;
};

constexpr usize MaxMemoryRegions = 128;
constexpr usize MaxModules = 8;

struct BootInfo {
    const char* loader_name = "";
    const char* loader_version = "";
    u64 protocol_revision = 0;         // Limine base revision actually used
    const char* cmdline = "";
    u64 hhdm_offset = 0;               // physical p is at virtual p + hhdm_offset (only for mapped types)
    kstd::PhysAddr kernel_phys_base;
    kstd::VirtAddr kernel_virt_base;
    kstd::PhysAddr rsdp;               // physical address of the ACPI RSDP, 0 if none
    bool uefi = false;
    u64 boot_time_unix = 0;            // seconds, 0 if unknown
    Framebuffer framebuffer;
    MemoryRegion memory[MaxMemoryRegions];
    usize memory_count = 0;
    Module modules[MaxModules];
    usize module_count = 0;

    kstd::Span<const MemoryRegion> memory_map() const { return {memory, memory_count}; }
};

const char* memory_type_name(MemoryType t);

}  // namespace boot

namespace arch {
// Reads the boot loader's responses into `info`. Returns false if the boot
// loader does not speak a protocol revision we can use; the caller must then
// stop without touching anything else.
bool collect_boot_info(boot::BootInfo& info);
}
