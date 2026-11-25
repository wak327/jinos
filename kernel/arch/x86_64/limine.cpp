// Limine boot protocol: the request table the boot loader fills in, and the
// translation of its responses into boot::BootInfo.
//
// Base revision 5 is requested: a restrictive direct map (only usable,
// reclaimable, kernel, framebuffer and firmware-table regions), RSDP given
// as a direct-map virtual address, strict register state and a well-defined
// local APIC. A loader that only speaks an older revision boots us with at
// least revision 3 and records which one; we accept 4 and 5 and refuse less.

#include <limine.h>

#include "core/boot_info.h"
#include "kstd/string.h"

namespace {

#define REQ __attribute__((used, section(".limine_requests")))

__attribute__((used, section(".limine_requests_start")))
constinit volatile u64 g_requests_start[4] = LIMINE_REQUESTS_START_MARKER;

REQ constinit volatile u64 g_base_revision[3] = LIMINE_BASE_REVISION(5);

REQ constinit volatile limine_bootloader_info_request g_info_req = {
    .id = LIMINE_BOOTLOADER_INFO_REQUEST_ID, .revision = 0, .response = nullptr};
REQ constinit volatile limine_executable_cmdline_request g_cmdline_req = {
    .id = LIMINE_EXECUTABLE_CMDLINE_REQUEST_ID, .revision = 0, .response = nullptr};
REQ constinit volatile limine_firmware_type_request g_firmware_req = {
    .id = LIMINE_FIRMWARE_TYPE_REQUEST_ID, .revision = 0, .response = nullptr};
REQ constinit volatile limine_stack_size_request g_stack_req = {
    .id = LIMINE_STACK_SIZE_REQUEST_ID, .revision = 0, .response = nullptr, .stack_size = 64 * kstd::KiB};
REQ constinit volatile limine_hhdm_request g_hhdm_req = {
    .id = LIMINE_HHDM_REQUEST_ID, .revision = 0, .response = nullptr};
REQ constinit volatile limine_framebuffer_request g_fb_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0, .response = nullptr};
REQ constinit volatile limine_memmap_request g_memmap_req = {
    .id = LIMINE_MEMMAP_REQUEST_ID, .revision = 0, .response = nullptr};
REQ constinit volatile limine_module_request g_module_req = {
    .id = LIMINE_MODULE_REQUEST_ID, .revision = 0, .response = nullptr,
    .internal_module_count = 0, .internal_modules = nullptr};
REQ constinit volatile limine_rsdp_request g_rsdp_req = {
    .id = LIMINE_RSDP_REQUEST_ID, .revision = 0, .response = nullptr};
REQ constinit volatile limine_date_at_boot_request g_date_req = {
    .id = LIMINE_DATE_AT_BOOT_REQUEST_ID, .revision = 0, .response = nullptr};
REQ constinit volatile limine_executable_address_request g_addr_req = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID, .revision = 0, .response = nullptr};

__attribute__((used, section(".limine_requests_end")))
constinit volatile u64 g_requests_end[2] = LIMINE_REQUESTS_END_MARKER;

#undef REQ

boot::MemoryType translate(u64 type) {
    switch (type) {
    case LIMINE_MEMMAP_USABLE: return boot::MemoryType::Usable;
    case LIMINE_MEMMAP_RESERVED: return boot::MemoryType::Reserved;
    case LIMINE_MEMMAP_RESERVED_MAPPED: return boot::MemoryType::ReservedMapped;
    case LIMINE_MEMMAP_ACPI_RECLAIMABLE: return boot::MemoryType::AcpiReclaimable;
    case LIMINE_MEMMAP_ACPI_NVS: return boot::MemoryType::AcpiNvs;
    case LIMINE_MEMMAP_BAD_MEMORY: return boot::MemoryType::Bad;
    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE: return boot::MemoryType::BootloaderReclaimable;
    case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES: return boot::MemoryType::KernelAndModules;
    case LIMINE_MEMMAP_FRAMEBUFFER: return boot::MemoryType::Framebuffer;
    default: return boot::MemoryType::Reserved;
    }
}

// The requests are volatile so the compiler keeps them; the responses are
// ordinary memory once the loader has written them.
template <typename T> T* response_of(const volatile T* volatile& p) { return const_cast<T*>(p); }

}  // namespace

namespace arch {

bool collect_boot_info(boot::BootInfo& info) {
    // Revision tag: [2] is zeroed if our revision was supported; otherwise
    // [1] holds the revision actually used (revision 3+ loaders).
    if (g_base_revision[2] == 0) {
        info.protocol_revision = 5;
    } else if (g_base_revision[1] != 0x6a7b384944536bdc) {
        info.protocol_revision = g_base_revision[1];
    } else {
        return false;  // revision 0-2 loader: no way to know what we got
    }
    if (info.protocol_revision < 4) return false;  // we rely on RSDP being virtual

    if (auto* r = response_of(g_info_req.response)) {
        info.loader_name = r->name;
        info.loader_version = r->version;
    }
    if (auto* r = response_of(g_cmdline_req.response); r && r->cmdline) info.cmdline = r->cmdline;
    if (auto* r = response_of(g_firmware_req.response)) {
        info.uefi = r->firmware_type == LIMINE_FIRMWARE_TYPE_EFI64 || r->firmware_type == LIMINE_FIRMWARE_TYPE_EFI32;
    }
    if (auto* r = response_of(g_hhdm_req.response)) info.hhdm_offset = r->offset;
    if (auto* r = response_of(g_addr_req.response)) {
        info.kernel_phys_base = kstd::PhysAddr(r->physical_base);
        info.kernel_virt_base = kstd::VirtAddr(r->virtual_base);
    }
    if (auto* r = response_of(g_rsdp_req.response); r && r->address) {
        // Revision 4+: a direct-map virtual address. Store it as physical.
        info.rsdp = kstd::PhysAddr(reinterpret_cast<u64>(r->address) - info.hhdm_offset);
    }
    if (auto* r = response_of(g_date_req.response); r && r->timestamp > 0) {
        info.boot_time_unix = u64(r->timestamp);
    }

    if (auto* r = response_of(g_fb_req.response); r && r->framebuffer_count > 0) {
        const limine_framebuffer* fb = r->framebuffers[0];
        if (fb->memory_model == LIMINE_FRAMEBUFFER_RGB && fb->bpp == 32) {
            auto& out = info.framebuffer;
            out.address = static_cast<u8*>(fb->address);
            out.width = u32(fb->width);
            out.height = u32(fb->height);
            out.pitch = u32(fb->pitch);
            out.bpp = fb->bpp;
            out.red_shift = fb->red_mask_shift;
            out.green_shift = fb->green_mask_shift;
            out.blue_shift = fb->blue_mask_shift;
            out.red_size = fb->red_mask_size;
            out.green_size = fb->green_mask_size;
            out.blue_size = fb->blue_mask_size;
        }
    }

    if (auto* r = response_of(g_memmap_req.response)) {
        usize n = kstd::min<usize>(r->entry_count, boot::MaxMemoryRegions);
        for (usize i = 0; i < n; i++) {
            const limine_memmap_entry* e = r->entries[i];
            info.memory[i] = {kstd::PhysAddr(e->base), e->length, translate(e->type)};
        }
        info.memory_count = n;
    }

    if (auto* r = response_of(g_module_req.response)) {
        usize n = kstd::min<usize>(r->module_count, boot::MaxModules);
        for (usize i = 0; i < n; i++) {
            const limine_file* f = r->modules[i];
            info.modules[i] = {f->address, f->size, f->path, f->string};
        }
        info.module_count = n;
    }
    return true;
}

}  // namespace arch

namespace boot {

const char* memory_type_name(MemoryType t) {
    switch (t) {
    case MemoryType::Usable: return "usable";
    case MemoryType::Reserved: return "reserved";
    case MemoryType::ReservedMapped: return "reserved (mapped)";
    case MemoryType::AcpiReclaimable: return "ACPI reclaimable";
    case MemoryType::AcpiNvs: return "ACPI NVS";
    case MemoryType::Bad: return "bad";
    case MemoryType::BootloaderReclaimable: return "loader reclaimable";
    case MemoryType::KernelAndModules: return "kernel + modules";
    case MemoryType::Framebuffer: return "framebuffer";
    }
    return "?";
}

}  // namespace boot
