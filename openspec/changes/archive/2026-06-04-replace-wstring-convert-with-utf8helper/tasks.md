## 1. utf8helper Enhancement

- [x] 1.1 Add `DecodeUTF8Codepoint` function declaration to `utf8helper.h`
- [x] 1.2 Add `EncodeCodepointToUTF8` function declaration to `utf8helper.h`
- [x] 1.3 Implement `DecodeUTF8Codepoint` in `utf8helper.cpp` using existing `Parser` state machine
- [x] 1.4 Implement `EncodeCodepointToUTF8` in `utf8helper.cpp`

## 2. String.cpp Rewrite

- [x] 2.1 Rewrite `UTF8StringToWString` to use `DecodeUTF8Codepoint` (with Windows wchar_t=2 surrogate handling)
- [x] 2.2 Rewrite `UTF8StringToU32String` to use `DecodeUTF8Codepoint`
- [x] 2.3 Rewrite `UTF8StringToU32StringLE` using `UTF8StringToU32String` + byte swap
- [x] 2.4 Rewrite `WStringToUTF8String` using `EncodeCodepointToUTF8` (with Windows wchar_t=2 surrogate assembly)
- [x] 2.5 Remove `<codecvt>` include and `HAVE_CODECVT` conditional from `String.cpp`

## 3. Build System Cleanup

- [x] 3.1 Remove `HAVE_CODECVT` detection from `cmake/Config.h.cmake`
- [x] 3.2 Remove `HAVE_CODECVT` detection from root `meson.build`
- [x] 3.3 Remove `HAVE_CODECVT` from `libosmscout/include/osmscout/private/meson.build` (Config.h template)

## 4. Testing and Validation

- [x] 4.1 Build and run existing `WStringStringConversionTest` — verify all tests pass
- [x] 4.2 Build with GCC 16.1.1 — verify no `wstring_convert`/`codecvt` deprecation warnings
- [x] 4.3 Full project build — verify no regressions in renderers that use these conversions
