## Why

`std::wstring_convert` (with `std::codecvt_utf8`) was deprecated in C++17 and removed in C++26. GCC 16.1.1 emits deprecation warnings for 4 functions in `String.cpp` that use it for UTF-8 encoding conversion. These warnings will become build errors on newer toolchains, breaking compilation.

## What Changes

- Replace `std::wstring_convert`/`std::codecvt_utf8` usage in `UTF8StringToWString`, `UTF8StringToU32String`, `UTF8StringToU32StringLE`, and `WStringToUTF8String` with equivalent conversions that do not use deprecated interfaces
- Add any missing conversion primitives to the existing UTF-8 infrastructure
- Remove `HAVE_CODECVT` configuration and `<codecvt>` include dependency
- Signatures of existing public API functions remain unchanged (no breaking changes)
- Callers in renderers (AGG, Cairo, GDI, DirectX, SVG, OpenGL, Qt) and Locale.cpp need no modification

## Capabilities

### New Capabilities
- `utf8-encoding-conversion`: Locale-independent UTF-8 to/from UTF-16 (`wstring`) and UTF-32 (`u32string`) conversions, supporting both native and little-endian byte order

### Modified Capabilities
*(none — existing string conversion APIs keep their signatures)*

## Impact

- **`libosmscout/src/osmscout/util/String.cpp`** — 4 functions rewritten to not use `wstring_convert`
- **`libosmscout/src/osmscout/util/utf8helper.h` / `utf8helper.cpp`** — possible additions for encoding conversion primitives
- **`libosmscout/include/osmscout/private/Config.h.in`** — remove `HAVE_CODECVT`
- **CMakeLists.txt / meson.build** — remove codecvt detection
- **All callers** — no changes needed (stable API)
- **Tests** — existing tests in `Tests/src/WStringStringConversionTest.cpp` must continue passing with identical results
