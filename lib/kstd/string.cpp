#include "kstd/string.h"

namespace kstd {

usize strlen(const char* s) {
    usize n = 0;
    while (s[n] != '\0') n++;
    return n;
}

usize strnlen(const char* s, usize max) {
    usize n = 0;
    while (n < max && s[n] != '\0') n++;
    return n;
}

int strcmp(const char* a, const char* b) {
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return int(u8(*a)) - int(u8(*b));
}

int strncmp(const char* a, const char* b, usize n) {
    for (usize i = 0; i < n; i++) {
        if (a[i] != b[i] || a[i] == '\0') return int(u8(a[i])) - int(u8(b[i]));
    }
    return 0;
}

bool contains(const char* haystack, const char* needle) {
    usize n = strlen(needle);
    if (n == 0) return true;
    for (; *haystack != '\0'; haystack++) {
        if (strncmp(haystack, needle, n) == 0) return true;
    }
    return false;
}

bool has_word(const char* s, const char* word) {
    usize n = strlen(word);
    if (n == 0 || s == nullptr) return false;
    while (*s != '\0') {
        while (*s == ' ') s++;
        const char* start = s;
        while (*s != '\0' && *s != ' ') s++;
        if (usize(s - start) == n && strncmp(start, word, n) == 0) return true;
    }
    return false;
}

}  // namespace kstd
