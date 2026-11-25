// Basic types for kernel code. Only C++20 freestanding headers are allowed
// here (docs/design.md 7.8).
#pragma once

#include <cstddef>
#include <cstdint>

namespace kstd {

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using usize = size_t;
using isize = ptrdiff_t;
using uptr = uintptr_t;

// Physical and virtual addresses are different things; keep them in
// different types so they cannot be mixed up silently.
struct PhysAddr {
    u64 value = 0;
    constexpr PhysAddr() = default;
    constexpr explicit PhysAddr(u64 v) : value(v) {}
    constexpr bool operator==(const PhysAddr&) const = default;
    constexpr PhysAddr operator+(u64 off) const { return PhysAddr(value + off); }
};

struct VirtAddr {
    u64 value = 0;
    constexpr VirtAddr() = default;
    constexpr explicit VirtAddr(u64 v) : value(v) {}
    constexpr explicit VirtAddr(const void* p) : value(reinterpret_cast<u64>(p)) {}
    constexpr bool operator==(const VirtAddr&) const = default;
    constexpr VirtAddr operator+(u64 off) const { return VirtAddr(value + off); }
    template <typename T> T* as() const { return reinterpret_cast<T*>(value); }
};

inline constexpr usize KiB = 1024;
inline constexpr usize MiB = 1024 * KiB;
inline constexpr usize GiB = 1024 * MiB;
inline constexpr usize PageSize = 4 * KiB;

template <typename T> constexpr T align_down(T v, usize a) { return T(u64(v) & ~u64(a - 1)); }
template <typename T> constexpr T align_up(T v, usize a) { return T((u64(v) + a - 1) & ~u64(a - 1)); }
template <typename T> constexpr T min(T a, T b) { return a < b ? a : b; }
template <typename T> constexpr T max(T a, T b) { return a > b ? a : b; }

}  // namespace kstd

using kstd::i16, kstd::i32, kstd::i64, kstd::i8;
using kstd::isize, kstd::uptr, kstd::usize;
using kstd::u16, kstd::u32, kstd::u64, kstd::u8;
