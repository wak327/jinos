// The console bitmap font, linked into the kernel as a PC Screen Font (PSF)
// file: third_party/fonts/ter-u16n-lat15.psf (Terminus 8x16, OFL).
#pragma once

#include "kstd/types.h"

namespace font {

struct Glyphs {
    const u8* data = nullptr;
    u32 count = 0;
    u32 width = 0, height = 0;
    u32 bytes_per_glyph = 0;
    u32 bytes_per_row = 0;

    // Rows of `bytes_per_row` bytes, most significant bit on the left.
    const u8* glyph(u8 ch) const {
        u32 i = ch < count ? ch : u32('?');
        return data + usize(i) * bytes_per_glyph;
    }
};

// Parses the embedded PSF1 or PSF2 file. height == 0 on failure.
Glyphs load();

}  // namespace font
