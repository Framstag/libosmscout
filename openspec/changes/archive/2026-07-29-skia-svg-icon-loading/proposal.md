## Why

The Skia map backend only loads PNG icons, ignoring SVG icons referenced by `.oss` stylesheets. SVG support means sharper icons at all zoom levels, smaller file sizes in stylesheets, and parity with other backends (Qt, Cairo) that support SVG.

## What Changes

- Add SVG file detection in `MapPainterSkia::HasIcon()` — try `.svg` extension when `.png` not found
- Add SVG rendering path in `MapPainterSkia::HasIcon()` — two code paths:
  - **Primary**: Skia's `SkSVGDOM` module (render to offscreen canvas, snapshot to `SkImage`)
  - **Fallback**: nanosvg (header-only C library, zlib license) when SkSVGDOM unavailable
- No change to `DrawIcon()` — cached result is always `sk_sp<SkImage>`, drawing code stays same
- Add CMake detection for Skia SVG module (header + symbol probe)
- Vendor nanosvg header for fallback path
- Add SVG module include path to `libosmscout-map-skia/CMakeLists.txt` (conditional)

## Capabilities

### New Capabilities
- `svg-icon-loading`: Load and render SVG format icons from stylesheet icon paths, caching rendered results alongside existing PNG icons

### Modified Capabilities
- *(none — no spec-level requirement changes)*

## Impact

**Affected files:**
- `libosmscout-map-skia/src/osmscoutmapskia/MapPainterSkia.cpp` — `HasIcon()` SVG loading logic
- `libosmscout-map-skia/CMakeLists.txt` — SVG module include dir and linkage
- `cmake/features.cmake` — optional SVG module detection
- `libosmscout-map-skia/include/osmscoutmapskia/MapSkiaFeatures.h.cmake` — optional feature flag

**Dependencies:**
- Skia SVG module (`modules/svg`) — provides `SkSVGDOM` class (primary path)
- nanosvg (vendored, zlib license) — fallback when SkSVGDOM unavailable
- **Risk:** SVG module may not be compiled into system Skia packages — CMake detection + nanosvg fallback handles this

**Not changing:**
- `DrawIcon()` method — signature and implementation stay unchanged
- `iconCache` type — remains `std::map<std::string, sk_sp<SkImage>>`
- Other backends or core library
