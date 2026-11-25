// panic(): the kernel cannot continue. Prints the reason on every output,
// draws the panic screen on the framebuffer (interrupts off, no allocation),
// and halts. Under `selftest` it instead asks QEMU to exit with a failure
// code so CI notices.
#pragma once

#include "kstd/types.h"

__attribute__((format(printf, 1, 2), noreturn)) void panic(const char* fmt, ...);

#define kassert(cond)                                                              \
    do {                                                                           \
        if (!(cond)) panic("assertion failed: %s (%s:%d)", #cond, __FILE__, __LINE__); \
    } while (0)

namespace panic_hooks {
// Called at the very end of panic(); may return (then the CPU halts).
void set_exit_hook(void (*hook)(bool failed));
}
