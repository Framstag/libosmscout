## 1. Cairo symbol rendering contract tests (spec: cairo-symbol-renderer)

- [ ] 1.1 Create `Tests/src/SymbolRendererCairoTest.cpp` with a test case per spec scenario (border width mm-to-px scaling, sub-pixel conversion, closed polygon outline via closing edge, fill and stroke emission, no-fill/no-border background, dashed border scaling), and verify each case asserts sampled pixels on a Cairo image surface as specified in the scenarios
- [ ] 1.2 Register `SymbolRendererCairoTest` in `Tests/CMakeLists.txt` with `osmscout_test_project` linking `OSMScout::MapCairo` (pattern: `MapPainterRouteTest`), and verify `cmake` configure lists the new test target
- [ ] 1.3 Register `SymbolRendererCairoTest` in `Tests/meson.build` (pattern: `SymbolRendererSVGTest` block), and verify `meson setup` accepts the registration

## 2. Verification

- [ ] 2.1 Build `SymbolRendererCairoTest` with cmake/Ninja, and verify it compiles without errors
- [ ] 2.2 Run `SymbolRendererCairoTest` via ctest, and verify all test cases pass
- [ ] 2.3 Run the existing test suite via ctest, and verify no regressions in unrelated areas
- [ ] 2.4 If a Meson build directory is available, compile and run the test there, and verify it passes (otherwise note Meson as unverified)
