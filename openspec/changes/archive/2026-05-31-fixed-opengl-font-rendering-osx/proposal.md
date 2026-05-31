## Why

OpenGL text rendering on macOS produces garbled characters because `UTF8StringToU32String` uses `std::codecvt_utf8<char32_t>` with default mode (native byte order). The FreeType/OpenGL pipeline expects a fixed byte order. We add a new method with explicit LE in the name using `std::codecvt_utf8<char32_t, 0x10ffff, std::little_endian>`.

## What Changes

- **New function** `UTF8StringToU32StringLE` in `libosmscout` — uses `std::codecvt_utf8` with `std::little_endian` flag on both MSVC and non-MSVC paths
- **Existing** `UTF8StringToU32String` stays unchanged — preserves correct native-byte-order behavior for other callers
- **OpenGL**: `TextLoader.cpp` calls `UTF8StringToU32StringLE` instead of `UTF8StringToU32String`
- **No new library dependencies** — pure `std::codecvt_utf8` with explicit mode flag

## Capabilities

### New Capabilities
- (none — existing function interface unchanged, new function is internal utility)

### Modified Capabilities
- (none — spec-level behavior unchanged, only internal detail)

## Impact

- **`libosmscout/include/osmscout/util/String.h`**: Add `UTF8StringToU32StringLE` declaration
- **`libosmscout/src/osmscout/util/String.cpp`**: Add `UTF8StringToU32StringLE` implementation (MSVC + non-MSVC paths, both with `std::little_endian`)
- **`libosmscout-map-opengl/src/osmscoutmapopengl/TextLoader.cpp`**: Change call from `UTF8StringToU32String` to `UTF8StringToU32StringLE`
- **Tests**: `Tests/src/WStringStringConversionTest.cpp` — add test for new function
