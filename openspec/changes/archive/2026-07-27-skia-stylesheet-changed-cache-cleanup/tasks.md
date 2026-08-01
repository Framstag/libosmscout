## 1. Add StyleSheetChanged declaration to header

- [ ] 1.1 Add `void StyleSheetChanged(const Projection& projection, const MapParameter& parameter, const std::vector<MapData>& data) override;` to `MapPainterSkia.h` protected section, alongside other overrides (`HasIcon`, `GetFontHeight`, `DrawGround`, etc.)

## 2. Implement StyleSheetChanged in MapPainterSkia.cpp

- [ ] 2.1 Implement `MapPainterSkia::StyleSheetChanged` — call `iconCache.clear()` and `patternCache.clear()`

## 3. Update TODO.md

- [ ] 3.1 Mark `StyleSheetChanged cache cleanup` item as `[DONE]` in `libosmscout-map-skia/TODO.md` (add note that implementation clears both caches)

## 4. Build and Test Verification

- [ ] 4.1 Verify CMake build compiles cleanly: `cmake -B build-skia-test && cmake --build build-skia-test`
- [ ] 4.2 Verify Meson build compiles cleanly: `meson setup build-meson-skia && meson compile -C build-meson-skia`
- [ ] 4.3 Run existing tests: `cd build-skia-test && ctest --output-on-failure`
