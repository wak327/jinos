// Thin wrappers over x86-64 instructions used by the kernel.
#pragma once

#include "kstd/types.h"

namespace arch {

inline void outb(u16 port, u8 v) { asm volatile("outb %0, %1" : : "a"(v), "Nd"(port) : "memory"); }
inline void outw(u16 port, u16 v) { asm volatile("outw %0, %1" : : "a"(v), "Nd"(port) : "memory"); }
inline void outl(u16 port, u32 v) { asm volatile("outl %0, %1" : : "a"(v), "Nd"(port) : "memory"); }
inline u8 inb(u16 port) {
    u8 v;
    asm volatile("inb %1, %0" : "=a"(v) : "Nd"(port) : "memory");
    return v;
}
inline u16 inw(u16 port) {
    u16 v;
    asm volatile("inw %1, %0" : "=a"(v) : "Nd"(port) : "memory");
    return v;
}
inline u32 inl(u16 port) {
    u32 v;
    asm volatile("inl %1, %0" : "=a"(v) : "Nd"(port) : "memory");
    return v;
}

inline void cli() { asm volatile("cli" ::: "memory"); }
inline void sti() { asm volatile("sti" ::: "memory"); }
inline void hlt() { asm volatile("hlt" ::: "memory"); }
inline void pause() { asm volatile("pause" ::: "memory"); }

// Stops this CPU for good. Interrupts off, so nothing wakes it.
[[noreturn]] inline void halt_forever() {
    cli();
    for (;;) hlt();
}

inline u64 read_cr0() { u64 v; asm volatile("mov %%cr0, %0" : "=r"(v)); return v; }
inline u64 read_cr2() { u64 v; asm volatile("mov %%cr2, %0" : "=r"(v)); return v; }
inline u64 read_cr3() { u64 v; asm volatile("mov %%cr3, %0" : "=r"(v)); return v; }
inline u64 read_cr4() { u64 v; asm volatile("mov %%cr4, %0" : "=r"(v)); return v; }

inline u64 rdtsc() {
    u32 lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return (u64(hi) << 32) | lo;
}

inline u64 rdmsr(u32 msr) {
    u32 lo, hi;
    asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return (u64(hi) << 32) | lo;
}
inline void wrmsr(u32 msr, u64 v) {
    asm volatile("wrmsr" : : "c"(msr), "a"(u32(v)), "d"(u32(v >> 32)) : "memory");
}

struct CpuidResult {
    u32 eax, ebx, ecx, edx;
};
inline CpuidResult cpuid(u32 leaf, u32 subleaf = 0) {
    CpuidResult r;
    asm volatile("cpuid" : "=a"(r.eax), "=b"(r.ebx), "=c"(r.ecx), "=d"(r.edx) : "a"(leaf), "c"(subleaf));
    return r;
}

}  // namespace arch
