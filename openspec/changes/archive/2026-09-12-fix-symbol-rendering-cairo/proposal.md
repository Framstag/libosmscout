## Why

The Cairo symbol renderer already implements the corrected behaviors on master — border widths are scaled by the mm-per-pixel factor, and polygon paths are closed before stroking — but, unlike the SVG and Skia backends, it has no unit test coverage at all. The SVG/Qt fixes landed in `fix-symbol-rendering-svg-qt`; for Cairo the contract exists only as untested implementation. Cairo is the most widely used rendering backend of this project (MCPServer, PerformanceTest, demos), so regressions in symbol rendering would be invisible to CI. This change locks the Cairo symbol rendering contract down with tests.

## What Changes

- New capability spec `cairo-symbol-renderer` documenting the observable behavior contract of `SymbolRendererCairo`: border width conversion from mm to pixels, closed polygon outlines, fill and stroke emission, dash scaling.
- New Catch2 unit test suite `SymbolRendererCairoTest` verifying the contract against rendered output (pixel-sampled assertions on a Cairo image surface), registered in both CMake and Meson.
- The tests surfaced a crash bug: `EndPrimitive()` dereferenced the border style without a null check, crashing on borderless symbols. This is fixed by guarding the computation; production change is limited to that guard.

## Capabilities

### New Capabilities

- `cairo-symbol-renderer`: contract for the Cairo backend's symbol rendering — mm-to-pixel border scaling, closed polygon outlines, fill/stroke emission, dash width scaling.

### Modified Capabilities

None.

## Impact

- `Tests/src/SymbolRendererCairoTest.cpp` — new test suite (new file)
- `Tests/CMakeLists.txt` — register `SymbolRendererCairoTest` (pattern: `TextMetricsCairoTest`, `MapPainterRouteTest` link `OSMScout::MapCairo`)
- `Tests/meson.build` — register the test for the Meson build
- `libosmscout-map-cairo/src/osmscoutmapcairo/SymbolRendererCairo.cpp` — guard `EndPrimitive()` against a null border style (crash found by the new tests)
- No changes to stylesheets or public APIs
