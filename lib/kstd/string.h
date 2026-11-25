// C string helpers and the mem* functions the compiler expects to exist.
#pragma once

#include "kstd/types.h"

extern "C" {
void* memcpy(void* dst, const void* src, usize n);
void* memmove(void* dst, const void* src, usize n);
void* memset(void* dst, int c, usize n);
int memcmp(const void* a, const void* b, usize n);
}

namespace kstd {

usize strlen(const char* s);
usize strnlen(const char* s, usize max);
int strcmp(const char* a, const char* b);
int strncmp(const char* a, const char* b, usize n);
// True if `needle` occurs in `haystack`.
bool contains(const char* haystack, const char* needle);
// True if `s` is `word` on its own, or appears in `s` separated by spaces.
// Used for boot command-line options.
bool has_word(const char* s, const char* word);

}  // namespace kstd
