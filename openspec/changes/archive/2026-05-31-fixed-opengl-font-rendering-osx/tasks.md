## 1. Declare new function

- [x] 1.1 Add `UTF8StringToU32StringLE` declaration to `libosmscout/include/osmscout/util/String.h` alongside existing `UTF8StringToU32String`

## 2. Implement new function

- [x] 2.1 Add `UTF8StringToU32StringLE` implementation to `libosmscout/src/osmscout/util/String.cpp` — MSVC path: `std::codecvt_utf8<int32_t, 0x10ffff, std::little_endian>` with `reinterpret_cast`. Non-MSVC path: `std::codecvt_utf8<char32_t, 0x10ffff, std::little_endian>`

## 3. Update OpenGL call site

- [x] 3.1 In `libosmscout-map-opengl/src/osmscoutmapopengl/TextLoader.cpp`, change `UTF8StringToU32String(text)` → `UTF8StringToU32StringLE(text)`

## 4. Add test

- [x] 4.1 Add test case for `UTF8StringToU32StringLE` in `Tests/src/WStringStringConversionTest.cpp`, verifying multi-byte UTF-8 sequences decode to correct codepoints