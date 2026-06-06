## Context

4 functions in `libosmscout/src/osmscout/util/String.cpp` use `std::wstring_convert<std::codecvt_utf8<...>>` for UTF-8 encoding conversion. This was deprecated in C++17 and removed in C++26. GCC 16.1.1 emits warnings.

The project already has UTF-8 parsing infrastructure in `utf8helper.h` / `utf8helper.cpp` with a state-machine-based `Parser` class that decodes multi-byte UTF-8 sequences to codepoints (`uint32_t`). However, this infrastructure was designed for text transformation (case folding, normalization, transliteration) — not for encoding conversion to `std::wstring` or `std::u32string`. Its internal storage is a `std::vector<codepoint>` with a raw-big-endian re-encoding, not standard UTF-8.

Key constraint: `wchar_t` is 4 bytes (UTF-32) on Linux/macOS, 2 bytes (UTF-16) on Windows/MSVC. The implementation must handle both.

## Goals / Non-Goals

**Goals:**
- Eliminate all `std::wstring_convert` / `<codecvt>` usage
- Preserve exact public API signatures and behavior
- Existing test suite passes with identical output
- No new external dependencies
- Use / enhance existing `utf8helper` infrastructure where practical
- Remove `HAVE_CODECVT` build configuration

**Non-Goals:**
- Change any public API signatures
- Modify any callers (renderers, Locale.cpp, etc.)
- Full UTF-8 validation beyond what the current code does
- Performance optimization beyond matching current behavior

## Decisions

### Decision 1: Add codepoint-level decoder/encoder to utf8helper

Add two small functions to `utf8helper` namespace, implemented in `utf8helper.cpp`:

```
// Decode one UTF-8 codepoint from text starting at offset.
// On success: stores codepoint in cp, advances offset past the encoded bytes, returns true
// On failure: offset unchanged, returns false
bool DecodeUTF8Codepoint(const std::string& text, size_t& offset, codepoint& cp);

// Encode one codepoint as UTF-8 and append to out
void EncodeCodepointToUTF8(codepoint cp, std::string& out);
```

These are generic, reusable primitives. They decode/encode standard UTF-8 (RFC 3629), not the internal raw-byte format of `UTF8String`. For characters in the pagemap tables, they use `character.code`. For characters outside the tables, they compute the codepoint correctly from the UTF-8 byte sequence (unlike the existing fallback in `_p1_u2`/`_p2_u3`/`_p3_u4` which stores raw shifted bytes).

**Alternatives considered:**
- *Write everything inline in String.cpp* — works but duplicates UTF-8 logic; utf8helper already has the Parser state machine
- *Use `UTF8String` class* — its internal storage uses raw-byte codepoints, not true codepoints for characters outside pagemap tables; cannot use directly for accurate conversion
- *Use iconv* — POSIX only, not portable to Windows without #ifdef
- *Use C11 `<uchar.h>`* — locale-dependent; not locale-independent

### Decision 2: Rewrite String.cpp using new utf8helper functions

Each of the 4 functions in String.cpp becomes a thin wrapper:

```
std::wstring UTF8StringToWString(const std::string& text) {
    std::wstring result;
    size_t offset = 0;
    codepoint cp;
    while (offset < text.size() && DecodeUTF8Codepoint(text, offset, cp)) {
        if (sizeof(wchar_t) == 2 && cp >= 0x10000) {
            // wchar_t is 2 bytes (UTF-16): encode as surrogate pair
            cp -= 0x10000;
            result.push_back(static_cast<wchar_t>(0xD800 | (cp >> 10)));
            result.push_back(static_cast<wchar_t>(0xDC00 | (cp & 0x3FF)));
        } else {
            result.push_back(static_cast<wchar_t>(cp));
        }
    }
    return result;
}
```

Similarly for `UTF8StringToU32String`, `UTF8StringToU32StringLE`, and `WStringToUTF8String`.

`UTF8StringToU32StringLE` is a special case: it converts to `std::u32string` with bytes in little-endian order. On little-endian hosts (x86, ARM), native `char32_t` byte order already matches LE, so no swap is needed. Only big-endian hosts need a byte swap.

```
std::u32string UTF8StringToU32StringLE(const std::string& text) {
    std::u32string u32 = UTF8StringToU32String(text);
    // Native byte order matches LE on LE hosts, swap only on BE.
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    for (auto& c : u32) {
        c = __builtin_bswap32(c);
    }
#endif
    return u32;
}
```

`WStringToUTF8String` reverses the process: iterates `wchar_t`, converts to codepoint (using `sizeof(wchar_t)` to detect UTF-16 vs UTF-32 and assemble surrogates accordingly), encodes to UTF-8 via `EncodeCodepointToUTF8`.

### Decision 3: Remove `HAVE_CODECVT`

- Remove `<codecvt>` include and `HAVE_CODECVT` conditional from String.cpp
- Remove codecvt detection from CMake `CMakeLists.txt` (find_package/module check)
- Remove codecvt detection from Meson `meson.build`
- Remove `HAVE_CODECVT` definition from `Config.h.in` templates

### Decision 4: Keep `<codecvt>` include guard for transitional safety (optional)

The current code has:
```
#if defined(HAVE_CODECVT)
#include <codecvt>
#else
static_assert(false, "Missing <codecvt> header...");
#endif
```

Since we no longer need `<codecvt>` at all, replace with unconditional removal. The `static_assert` can be removed.

## Risks / Trade-offs

- **[Platform] wchar_t size difference** — Must test on both Linux (4-byte wchar_t) and Windows (2-byte wchar_t). The code uses `sizeof(wchar_t)` at compile time to select logic — no OS #ifdefs needed, works on all platforms including macOS.
- **[Correctness] UTF-8 validation** — The current code uses `wstring_convert` which silently replaces invalid sequences. Our implementation must match this behavior (skip invalid sequences, not crash). The `DecodeUTF8Codepoint` function returns false on invalid bytes.
- **[Backward compat] U32StringLE** — `codecvt_utf8<char32_t, little_endian>` outputs LE byte order. On LE hosts (x86, ARM), native `char32_t` order is already LE, so no swap is needed. Swap only on BE hosts. The existing test confirms this on x86.
- **[Build] Both CMake and Meson** — Both build systems must be updated in sync. Forgetting one breaks CI on that system.
- **[Build] stale Config.h** — Generated `Config.h` files in existing build directories still have `HAVE_CODECVT`. Clean build required or rebuild will break.

## UTF-8 Encoding / Decoding Reference

```
Decode (byte sequence → codepoint):
  1-byte:  0xxxxxxx                    →  U+0000–U+007F
  2-byte:  110xxxxx 10xxxxxx           →  U+0080–U+07FF
  3-byte:  1110xxxx 10xxxxxx 10xxxxxx  →  U+0800–U+FFFF
  4-byte:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx  →  U+10000–U+10FFFF

Encode (codepoint → byte sequence):
  U+0000–U+007F:   0xxxxxxx
  U+0080–U+07FF:   110xxxxx 10xxxxxx
  U+0800–U+FFFF:   1110xxxx 10xxxxxx 10xxxxxx
  U+10000–U+10FFFF: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
```

## Migration Plan

1. Add `DecodeUTF8Codepoint` and `EncodeCodepointToUTF8` to `utf8helper.h`/`utf8helper.cpp`
2. Rewrite 4 functions in `String.cpp` to use new helpers
3. Remove `HAVE_CODECVT` and `<codecvt>` include from `String.cpp`
4. Update CMake build: remove codecvt detection
5. Update Meson build: remove codecvt detection
6. Remove `HAVE_CODECVT` from `Config.h.in` templates
7. Run existing tests — verify identical output
8. Clean build on GCC 16.1.1 — verify no deprecation warnings
