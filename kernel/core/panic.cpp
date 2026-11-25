#include "core/panic.h"

#include <cstdarg>

#include "arch/x86_64/cpu.h"
#include "core/fbcon.h"
#include "core/print.h"
#include "kstd/string.h"

namespace {
constinit void (*g_exit_hook)(bool failed) = nullptr;
constinit bool g_panicking = false;
}

namespace panic_hooks {
void set_exit_hook(void (*hook)(bool failed)) { g_exit_hook = hook; }
}

void panic(const char* fmt, ...) {
    arch::cli();
    if (g_panicking) arch::halt_forever();  // a panic inside the panic path
    g_panicking = true;

    char msg[512];
    va_list args;
    va_start(args, fmt);
    print::vsnprintf(msg, sizeof msg, fmt, args);
    va_end(args);

    // Serial first: it cannot fail in interesting ways.
    print::printf("\n*** PANIC: %s\n", msg);

    if (fbcon::present()) {
        // A red band across the top of the screen, message in white. Drawn
        // without touching anything but the framebuffer.
        fbcon::set_colors({0xff, 0xff, 0xff}, {0xb0, 0x1c, 0x1c});
        fbcon::fill_rows(0, 4, {0xb0, 0x1c, 0x1c});
        fbcon::write("\n  Siraj panic: ", 16);
        fbcon::write(msg, kstd::strlen(msg));
        fbcon::write("\n", 1);
    }

    if (g_exit_hook) g_exit_hook(true);
    arch::halt_forever();
}
