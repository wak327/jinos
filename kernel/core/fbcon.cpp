#include "core/fbcon.h"

#include "core/font.h"
#include "kstd/string.h"

namespace fbcon {
namespace {

// Big enough for 4K at 8x16 (480x135); larger screens use the top-left part.
constexpr u32 MaxCols = 512;
constexpr u32 MaxRows = 160;

struct Cell {
    char ch;
    u8 attr;   // index into g_palette pairs, 0 = default
};

struct State {
    boot::Framebuffer fb;
    font::Glyphs font;
    u32 cols = 0, rows = 0;
    u32 cur_col = 0, cur_row = 0;
    Color fg{0xe8, 0xe8, 0xea};
    Color bg{0x1e, 0x1f, 0x24};
    bool ready = false;
};

constinit State g_s;
constinit Cell g_cells[MaxRows][MaxCols];

u32 pack(const boot::Framebuffer& fb, Color c) {
    auto part = [](u8 v, u8 size, u8 shift) -> u32 {
        u32 x = size >= 8 ? u32(v) : (u32(v) >> (8 - size));
        return x << shift;
    };
    return part(c.r, fb.red_size, fb.red_shift) | part(c.g, fb.green_size, fb.green_shift) |
           part(c.b, fb.blue_size, fb.blue_shift);
}

void draw_glyph(u32 col, u32 row, char ch, Color fg, Color bg) {
    const auto& fb = g_s.fb;
    const u8* glyph = g_s.font.glyph(u8(ch));
    u32 fgp = pack(fb, fg), bgp = pack(fb, bg);
    u32 x0 = col * g_s.font.width, y0 = row * g_s.font.height;
    for (u32 y = 0; y < g_s.font.height; y++) {
        auto* line = reinterpret_cast<u32*>(fb.address + usize(y0 + y) * fb.pitch) + x0;
        u32 bits = 0;
        for (u32 b = 0; b < g_s.font.bytes_per_row; b++) bits = (bits << 8) | glyph[y * g_s.font.bytes_per_row + b];
        u32 top_bit = g_s.font.bytes_per_row * 8 - 1;
        for (u32 x = 0; x < g_s.font.width; x++) {
            line[x] = (bits >> (top_bit - x)) & 1 ? fgp : bgp;
        }
    }
}

void redraw_row(u32 row) {
    for (u32 c = 0; c < g_s.cols; c++) draw_glyph(c, row, g_cells[row][c].ch, g_s.fg, g_s.bg);
}

void scroll() {
    for (u32 r = 1; r < g_s.rows; r++) memcpy(g_cells[r - 1], g_cells[r], sizeof(Cell) * g_s.cols);
    for (u32 c = 0; c < g_s.cols; c++) g_cells[g_s.rows - 1][c] = {' ', 0};
    // Redrawing the whole grid is pure writes to write-combining memory,
    // which is fast; reading the framebuffer back would not be.
    for (u32 r = 0; r < g_s.rows; r++) redraw_row(r);
}

void newline() {
    g_s.cur_col = 0;
    if (++g_s.cur_row == g_s.rows) {
        g_s.cur_row = g_s.rows - 1;
        scroll();
    }
}

void put(char ch) {
    if (ch == '\n') {
        newline();
        return;
    }
    if (ch == '\r') {
        g_s.cur_col = 0;
        return;
    }
    if (ch == '\t') {
        do put(' ');
        while (g_s.cur_col % 8 != 0);
        return;
    }
    if (u8(ch) < 0x20) ch = '?';
    if (g_s.cur_col == g_s.cols) newline();
    g_cells[g_s.cur_row][g_s.cur_col] = {ch, 0};
    draw_glyph(g_s.cur_col, g_s.cur_row, ch, g_s.fg, g_s.bg);
    g_s.cur_col++;
}

}  // namespace

bool init(const boot::Framebuffer& fb) {
    if (!fb.present() || fb.bpp != 32) return false;
    g_s.fb = fb;
    g_s.font = font::load();
    if (g_s.font.height == 0) return false;
    g_s.cols = kstd::min(fb.width / g_s.font.width, MaxCols);
    g_s.rows = kstd::min(fb.height / g_s.font.height, MaxRows);
    if (g_s.cols == 0 || g_s.rows == 0) return false;
    g_s.ready = true;
    clear();
    return true;
}

bool present() { return g_s.ready; }

void write(const char* s, usize n) {
    if (!g_s.ready) return;
    for (usize i = 0; i < n; i++) put(s[i]);
}

void set_colors(Color fg, Color bg) {
    g_s.fg = fg;
    g_s.bg = bg;
}

void clear() {
    if (!g_s.ready) return;
    for (u32 r = 0; r < g_s.rows; r++)
        for (u32 c = 0; c < g_s.cols; c++) g_cells[r][c] = {' ', 0};
    u32 bgp = pack(g_s.fb, g_s.bg);
    for (u32 y = 0; y < g_s.fb.height; y++) {
        auto* line = reinterpret_cast<u32*>(g_s.fb.address + usize(y) * g_s.fb.pitch);
        for (u32 x = 0; x < g_s.fb.width; x++) line[x] = bgp;
    }
    g_s.cur_col = g_s.cur_row = 0;
}

void fill_rows(u32 first_row, u32 rows, Color bg) {
    if (!g_s.ready) return;
    u32 y_end = kstd::min((first_row + rows) * g_s.font.height, g_s.fb.height);
    u32 bgp = pack(g_s.fb, bg);
    for (u32 y = first_row * g_s.font.height; y < y_end; y++) {
        auto* line = reinterpret_cast<u32*>(g_s.fb.address + usize(y) * g_s.fb.pitch);
        for (u32 x = 0; x < g_s.fb.width; x++) line[x] = bgp;
    }
    for (u32 r = first_row; r < first_row + rows && r < g_s.rows; r++)
        for (u32 c = 0; c < g_s.cols; c++) g_cells[r][c] = {' ', 0};
    g_s.cur_row = first_row;
    g_s.cur_col = 0;
}

u32 columns() { return g_s.cols; }
u32 rows() { return g_s.rows; }

}  // namespace fbcon
