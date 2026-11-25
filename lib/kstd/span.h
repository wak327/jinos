#pragma once

#include "kstd/types.h"

namespace kstd {

// A view over a contiguous run of T. Never owns.
template <typename T>
class Span {
public:
    constexpr Span() = default;
    constexpr Span(T* data, usize size) : data_(data), size_(size) {}
    template <usize N>
    constexpr Span(T (&arr)[N]) : data_(arr), size_(N) {}

    constexpr T* data() const { return data_; }
    constexpr usize size() const { return size_; }
    constexpr bool empty() const { return size_ == 0; }
    constexpr T& operator[](usize i) const { return data_[i]; }
    constexpr T* begin() const { return data_; }
    constexpr T* end() const { return data_ + size_; }

    constexpr Span subspan(usize offset, usize count) const {
        return Span(data_ + offset, count);
    }
    constexpr Span first(usize count) const { return Span(data_, count); }

private:
    T* data_ = nullptr;
    usize size_ = 0;
};

}  // namespace kstd
