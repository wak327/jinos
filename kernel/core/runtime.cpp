// The little C++ runtime the kernel needs: stack-protector hooks, pure
// virtual trap, and the PSF font parser (kept here because it is runtime
// data, not logic).

#include "core/font.h"
#include "core/panic.h"
#include "kstd/types.h"

// --- Stack protector -------------------------------------------------------
// -mstack-protector-guard=global: the compiler compares against this symbol.
// S1 replaces the constant with a random value before anything untrusted
// runs; until then it only catches accidents.
extern "C" {
__attribute__((used)) uptr __stack_chk_guard = 0x595e9fbd94fda766;

[[noreturn]] void __stack_chk_fail() { panic("stack smashing detected"); }

[[noreturn]] void __cxa_pure_virtual() { panic("pure virtual function called"); }
}

// --- Font --------------------------------------------------------------------
extern "C" const u8 g_font_psf[];
extern "C" const u8 g_font_psf_end[];

namespace font {

namespace {
u32 le32(const u8* p) { return u32(p[0]) | u32(p[1]) << 8 | u32(p[2]) << 16 | u32(p[3]) << 24; }
}

Glyphs load() {
    Glyphs g;
    const u8* p = g_font_psf;
    usize size = usize(g_font_psf_end - g_font_psf);
    if (size >= 4 && p[0] == 0x36 && p[1] == 0x04) {
        // PSF1: 8 pixels wide, `charsize` rows, 256 or 512 glyphs.
        g.width = 8;
        g.height = p[3];
        g.count = (p[2] & 0x01) ? 512 : 256;
        g.bytes_per_glyph = g.height;
        g.bytes_per_row = 1;
        g.data = p + 4;
    } else if (size >= 32 && le32(p) == 0x864ab572) {
        // PSF2.
        u32 header = le32(p + 8);
        g.count = le32(p + 16);
        g.bytes_per_glyph = le32(p + 20);
        g.height = le32(p + 24);
        g.width = le32(p + 28);
        g.bytes_per_row = (g.width + 7) / 8;
        g.data = p + header;
    } else {
        return {};
    }
    if (g.data + usize(g.count) * g.bytes_per_glyph > g_font_psf_end || g.width == 0 || g.height == 0) return {};
    return g;
}

}  // namespace font
