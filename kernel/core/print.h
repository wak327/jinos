// Kernel text output. Everything goes to every registered sink (serial, the
// framebuffer console). printf-style, with the subset a kernel needs:
//   %c %s %d %i %u %x %X %p %%   with l/ll/z length modifiers,
//   a field width, '0' and '-' flags, and '#' for a 0x prefix on %x.
// No floating point: the kernel has none.
#pragma once

#include <cstdarg>

#include "kstd/types.h"

namespace print {

struct Sink {
    void (*write)(const char* s, usize n) = nullptr;
};

constexpr usize MaxSinks = 4;
void add_sink(Sink sink);

void write(const char* s, usize n);
void puts(const char* s);

__attribute__((format(printf, 1, 2))) void printf(const char* fmt, ...);
void vprintf(const char* fmt, va_list args);

// Formats into a caller-provided buffer, always NUL-terminated. Returns the
// length the full output would have had (like snprintf).
__attribute__((format(printf, 3, 4))) usize snprintf(char* buf, usize size, const char* fmt, ...);
usize vsnprintf(char* buf, usize size, const char* fmt, va_list args);

}  // namespace print

// The kernel log. Lines are prefixed with the subsystem name.
#define klog(subsystem, fmt, ...) ::print::printf(subsystem ": " fmt "\n", ##__VA_ARGS__)
