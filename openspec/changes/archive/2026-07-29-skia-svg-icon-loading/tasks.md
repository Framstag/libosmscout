# Tasks — SVG Icon Loading

## Task 1: Vendor nanosvg header [x]

Copy `nanosvg.h` from https://github.com/memononen/nanosvg into `libosmscout-map-skia/src/osmscoutmapskia/nanosvg.h`.

**Files:**
- `libosmscout-map-skia/src/osmscoutmapskia/nanosvg.h` (new, vendored)

**Acceptance:** Header compiles when included with `#define NANOSVG_IMPLEMENTATION` before `#include`.

---

## Task 2: Add Skia SVG module detection to CMake [x]

Add probe in `cmake/features.cmake` to detect whether the Skia SVG module is available (header + symbols in libskia). Set `OSMSCOUT_HAVE_SKIA_SVG` and `SVG_INCLUDE_DIRS`.

Add `#cmakedefine OSMSCOUT_HAVE_SKIA_SVG` to `MapSkiaFeatures.h.cmake`.

Add conditional include dir and compile definition to `libosmscout-map-skia/CMakeLists.txt`.

**Files:**
- `cmake/features.cmake` — SVG module detection
- `libosmscout-map-skia/include/osmscoutmapskia/MapSkiaFeatures.h.cmake` — feature define
- `libosmscout-map-skia/CMakeLists.txt` — conditional include + define

**Acceptance:**
- `cmake -B build && cmake --build build` succeeds with and without Skia SVG module
- `MapSkiaFeatures.h` contains `#define OSMSCOUT_HAVE_SKIA_SVG` when SVG module found
- `MapSkiaFeatures.h` does NOT contain the define when SVG module absent

---

## Task 3: Implement SVG loading in HasIcon() [x]

Extend `MapPainterSkia::HasIcon()` in `MapPainterSkia.cpp`:

1. After PNG probe fails, try `.svg` extension
2. If `OSMSCOUT_HAVE_SKIA_SVG` is defined:
   - Open file with `SkFILEStream`
   - Parse with `SkSVGDOM::Make(stream)`
   - Create offscreen `SkSurface`, render SVG, snapshot to `SkImage`
   - Cache in `iconCache`
3. Else (nanosvg fallback):
   - Parse with `nsvgParseFromFile`
   - Rasterize with `nsvgRasterize` to RGBA buffer
   - Wrap as `SkImage` via `SkImages::RasterFromData`
   - Cache in `iconCache`
4. Handle scaling per `IconMode` in both paths

**Files:**
- `libosmscout-map-skia/src/osmscoutmapskia/MapPainterSkia.cpp` — `HasIcon()` changes

**Acceptance:**
- SVG icons render correctly in both SkSVGDOM and nanosvg paths
- PNG icons still load (regression test)
- Missing SVG files handled gracefully (no crash)
- Malformed SVG files handled gracefully (no crash)
- `IconMode::Scalable` scales SVG to configured size
- `IconMode::OriginalPixmap` uses native SVG size

---

## Task 4: Verify build and run tests [x]

Build the project with and without Skia SVG module available. Run existing Skia map backend tests.

**Commands:**
```bash
cmake -B build -DOSMSCOUT_BUILD_MAP_SKIA=ON
cmake --build build
cd build && ctest -R "MapPainterSkia|SymbolRendererSkia" --output-on-failure
```

**Acceptance:** All existing tests pass. No new warnings.
