#include "core/print.h"

#include "kstd/string.h"

namespace print {
namespace {

constinit Sink g_sinks[MaxSinks];
constinit usize g_sink_count = 0;

// Accumulates formatted output, either into a bounded buffer or straight to
// the sinks (via a small staging buffer so sinks see whole chunks).
struct Out {
    char* buf;        // nullptr: write to sinks
    usize cap;        // buffer capacity including the NUL
    usize len = 0;    // characters produced (not necessarily stored)
    char stage[128];
    usize staged = 0;

    void put(char c) {
        if (buf) {
            if (len + 1 < cap) buf[len] = c;
        } else {
            stage[staged++] = c;
            if (staged == sizeof stage) flush();
        }
        len++;
    }
    void put(const char* s, usize n) {
        for (usize i = 0; i < n; i++) put(s[i]);
    }
    void flush() {
        if (!buf && staged > 0) {
            write(stage, staged);
            staged = 0;
        }
    }
    void finish() {
        if (buf && cap > 0) buf[len < cap ? len : cap - 1] = '\0';
        flush();
    }
};

struct Spec {
    bool left = false;
    bool zero = false;
    bool alt = false;
    usize width = 0;
};

void pad(Out& out, const Spec& spec, usize body_len, bool before) {
    if (spec.width <= body_len) return;
    char fill = (spec.zero && !spec.left) ? '0' : ' ';
    if (before != spec.left) {
        for (usize i = body_len; i < spec.width; i++) out.put(fill);
    }
}

void emit_number(Out& out, Spec spec, u64 value, bool negative, unsigned base, bool upper) {
    char digits[24];
    usize n = 0;
    const char* alphabet = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    do {
        digits[n++] = alphabet[value % base];
        value /= base;
    } while (value != 0);

    char prefix[3];
    usize prefix_len = 0;
    if (negative) prefix[prefix_len++] = '-';
    if (spec.alt && base == 16) {
        prefix[prefix_len++] = '0';
        prefix[prefix_len++] = upper ? 'X' : 'x';
    }

    usize body = prefix_len + n;
    if (spec.zero && !spec.left) {
        // Zero padding goes between the prefix and the digits.
        out.put(prefix, prefix_len);
        for (usize i = body; i < spec.width; i++) out.put('0');
    } else {
        pad(out, spec, body, true);
        out.put(prefix, prefix_len);
    }
    while (n > 0) out.put(digits[--n]);
    if (spec.left) pad(out, spec, body, false);
}

void format(Out& out, const char* fmt, va_list args) {
    for (const char* p = fmt; *p != '\0'; p++) {
        if (*p != '%') {
            out.put(*p);
            continue;
        }
        p++;
        Spec spec;
        for (;; p++) {
            if (*p == '-') spec.left = true;
            else if (*p == '0') spec.zero = true;
            else if (*p == '#') spec.alt = true;
            else break;
        }
        while (*p >= '0' && *p <= '9') spec.width = spec.width * 10 + usize(*p++ - '0');
        int longs = 0;
        bool size_t_arg = false;
        for (;; p++) {
            if (*p == 'l') longs++;
            else if (*p == 'z') size_t_arg = true;
            else break;
        }

        auto take_unsigned = [&]() -> u64 {
            if (size_t_arg) return va_arg(args, usize);
            if (longs >= 1) return va_arg(args, unsigned long);
            return va_arg(args, unsigned int);
        };
        auto take_signed = [&]() -> i64 {
            if (size_t_arg) return va_arg(args, isize);
            if (longs >= 1) return va_arg(args, long);
            return va_arg(args, int);
        };

        switch (*p) {
        case '%': out.put('%'); break;
        case 'c': {
            char c = char(va_arg(args, int));
            pad(out, spec, 1, true);
            out.put(c);
            if (spec.left) pad(out, spec, 1, false);
            break;
        }
        case 's': {
            const char* s = va_arg(args, const char*);
            if (s == nullptr) s = "(null)";
            usize n = kstd::strlen(s);
            pad(out, spec, n, true);
            out.put(s, n);
            if (spec.left) pad(out, spec, n, false);
            break;
        }
        case 'd':
        case 'i': {
            i64 v = take_signed();
            bool neg = v < 0;
            u64 mag = neg ? (~u64(v) + 1) : u64(v);
            emit_number(out, spec, mag, neg, 10, false);
            break;
        }
        case 'u': emit_number(out, spec, take_unsigned(), false, 10, false); break;
        case 'x': emit_number(out, spec, take_unsigned(), false, 16, false); break;
        case 'X': emit_number(out, spec, take_unsigned(), false, 16, true); break;
        case 'p': {
            spec.alt = true;
            if (spec.width == 0) {
                spec.width = 18;
                spec.zero = true;
            }
            emit_number(out, spec, reinterpret_cast<u64>(va_arg(args, void*)), false, 16, false);
            break;
        }
        case '\0': return;
        default:
            out.put('%');
            out.put(*p);
            break;
        }
    }
}

}  // namespace

void add_sink(Sink sink) {
    if (g_sink_count < MaxSinks) g_sinks[g_sink_count++] = sink;
}

void write(const char* s, usize n) {
    for (usize i = 0; i < g_sink_count; i++) g_sinks[i].write(s, n);
}

void puts(const char* s) { write(s, kstd::strlen(s)); }

void vprintf(const char* fmt, va_list args) {
    Out out{nullptr, 0};
    format(out, fmt, args);
    out.finish();
}

void printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

usize vsnprintf(char* buf, usize size, const char* fmt, va_list args) {
    Out out{buf, size};
    format(out, fmt, args);
    out.finish();
    return out.len;
}

usize snprintf(char* buf, usize size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    usize n = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return n;
}

}  // namespace print
