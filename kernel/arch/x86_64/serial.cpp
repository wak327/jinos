#include "arch/x86_64/serial.h"

#include "arch/x86_64/cpu.h"

namespace arch::serial {
namespace {

constexpr u16 COM1 = 0x3f8;

// Register offsets.
constexpr u16 DATA = 0;         // RBR/THR (DLAB=0), DLL (DLAB=1)
constexpr u16 IER = 1;          // interrupt enable (DLAB=0), DLM (DLAB=1)
constexpr u16 FCR = 2;          // FIFO control
constexpr u16 LCR = 3;          // line control
constexpr u16 MCR = 4;          // modem control
constexpr u16 LSR = 5;          // line status
constexpr u16 SCRATCH = 7;

constexpr u8 LSR_THR_EMPTY = 1 << 5;

constinit bool g_present = false;

}  // namespace

bool init() {
    // A missing UART reads 0xff from every port; the scratch register is a
    // cheap way to tell.
    outb(COM1 + SCRATCH, 0x5a);
    if (inb(COM1 + SCRATCH) != 0x5a) return false;

    outb(COM1 + IER, 0x00);         // no interrupts
    outb(COM1 + LCR, 0x80);         // DLAB on
    outb(COM1 + DATA, 0x01);        // divisor 1: 115200 baud
    outb(COM1 + IER, 0x00);
    outb(COM1 + LCR, 0x03);         // 8 data bits, no parity, 1 stop bit, DLAB off
    outb(COM1 + FCR, 0xc7);         // FIFOs on, cleared, 14-byte threshold
    outb(COM1 + MCR, 0x0b);         // DTR, RTS, OUT2

    g_present = true;
    return true;
}

bool present() { return g_present; }

void put(char c) {
    if (!g_present) return;
    // Bounded wait: a wedged UART must not hang the kernel.
    for (u32 i = 0; i < 100000 && (inb(COM1 + LSR) & LSR_THR_EMPTY) == 0; i++) pause();
    outb(COM1 + DATA, u8(c));
}

void write(const char* s, usize n) {
    for (usize i = 0; i < n; i++) {
        if (s[i] == '\n') put('\r');
        put(s[i]);
    }
}

}  // namespace arch::serial
