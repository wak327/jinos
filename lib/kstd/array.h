#pragma once

#include "kstd/span.h"
#include "kstd/types.h"

namespace kstd {

template <typename T, usize N>
struct Array {
    T items[N];

    constexpr T& operator[](usize i) { return items[i]; }
    constexpr const T& operator[](usize i) const { return items[i]; }
    constexpr usize size() const { return N; }
    constexpr T* data() { return items; }
    constexpr const T* data() const { return items; }
    constexpr T* begin() { return items; }
    constexpr T* end() { return items + N; }
    constexpr const T* begin() const { return items; }
    constexpr const T* end() const { return items + N; }
    constexpr Span<T> span() { return Span<T>(items, N); }
    constexpr Span<const T> span() const { return Span<const T>(items, N); }
};

}  // namespace kstd
