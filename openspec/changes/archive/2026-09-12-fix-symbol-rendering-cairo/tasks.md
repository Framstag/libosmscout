## 1. Cairo symbol rendering contract tests (spec: cairo-symbol-renderer)

- [x] 1.1 Create `Tests/src/SymbolRendererCairoTest.cpp` with a test case per spec scenario (border width mm-to-px scaling, sub-pixel conversion, closed polygon outline via closing edge, fill and stroke emission, no-fill/no-border background, dashed border scaling) — verified via ctest: 6/6 pass; also exposed and fixed a null-border crash in `EndPrimitive()`
- [x] 1.2 Register `SymbolRendererCairoTest` in `Tests/CMakeLists.txt` — verified: target builds and runs via ctest
- [ ] 1.3 Register `SymbolRendererCairoTest` in `Tests/meson.build` (registration written; Meson build dir not available on this machine, so `meson setup` verification pending)

## 2. Verification

- [x] 2.1 Build `SymbolRendererCairoTest` with cmake/Ninja — compiled without errors
- [x] 2.2 Run `SymbolRendererCairoTest` via ctest — 6/6 test cases pass
- [x] 2.3 Run the existing test suite via ctest — 111/111 passed, no regressions
- [ ] 2.4 Meson verification: no Meson build directory available, noted as unverified
