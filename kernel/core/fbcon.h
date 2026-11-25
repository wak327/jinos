// Text console on the boot framebuffer. Keeps a character grid so that
// scrolling redraws from RAM instead of reading back from the (uncached)
// framebuffer. Used for the boot log and the panic screen; the compositor
// replaces it later.
#pragma once

#include "core/boot_info.h"
#include "kstd/types.h"

namespace fbcon {

struct Color {
    u8 r, g, b;
};

// Returns false if there is no usable framebuffer.
bool init(const boot::Framebuffer& fb);
bool present();
void write(const char* s, usize n);
void set_colors(Color fg, Color bg);
// Clears the screen with the current background colour and homes the cursor.
void clear();
// Fills `rows` text rows from the top with `bg`; the panic screen uses it.
void fill_rows(u32 first_row, u32 rows, Color bg);
u32 columns();
u32 rows();

}  // namespace fbcon
