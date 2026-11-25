// 16550 UART on COM1. The first thing that works at boot, and the main
// debug channel under QEMU (`-serial stdio`).
#pragma once

#include "kstd/types.h"

namespace arch::serial {

// Returns false if no UART responded (real laptops rarely have one).
bool init();
bool present();
void put(char c);
void write(const char* s, usize n);

}  // namespace arch::serial
