## 1. Directory Structure & Build Files

- [x] 1.1 Create `libosmscout-map-skia/` directory with `include/osmscoutmapskia/` and `src/osmscoutmapskia/` subdirectories
- [x] 1.2 Create `libosmscout-map-skia/CMakeLists.txt` with `osmscout_library_project()` call for `OSMScoutMapSkia` (alias `MapSkia`, output `osmscout_map_skia`) linking `OSMScout::OSMScout`, `OSMScout::Map`, and `Skia::skia`
- [x] 1.3 Create `libosmscout-map-skia/meson.build` with library target `osmscout_map_skia` linking `osmscoutmap`, `osmscout`, and `skiaDep`
- [x] 1.4 Create `libosmscout-map-skia/include/meson.build` with header list and install rules
- [x] 1.5 Create `libosmscout-map-skia/src/meson.build` with source file list
- [x] 1.6 Create `libosmscout-map-skia/include/osmscoutmapskia/MapSkiaImportExport.h` with `OSMSCOUT_MAP_SKIA_API` export/import macro
- [x] 1.7 Create `libosmscout-map-skia/include/osmscoutmapskia/MapSkiaFeatures.h.cmake` with feature detection header template

## 2. Root Build System Integration (CMake)

- [x] 2.1 Add `find_package(Skia)` or `FindSkia.cmake` module in `cmake/features.cmake`
- [x] 2.2 Add `OSMSCOUT_BUILD_MAP_SKIA` option block in root `CMakeLists.txt` (precondition check + `add_subdirectory(libosmscout-map-skia)`) — follow AGG/Cairo pattern
- [x] 2.3 Verify `OSMSCOUT_BUILD_MAP_SKIA=ON` builds `libosmscout_map_skia` successfully

## 3. Root Build System Integration (Meson)

- [x] 3.1 Add `enableMapSkia` option in `meson_options.txt`
- [x] 3.2 Add Skia dependency detection in root `meson.build` (`dependency('skia', required: false)`)
- [x] 3.3 Add `buildMapSkia` conditional block and `subdir('libosmscout-map-skia')` in root `meson.build`
- [x] 3.4 Verify `-DenableMapSkia=true` builds `libosmscout_map_skia` successfully

## 4. MapPainterSkia Header

- [x] 4.1 Create `include/osmscoutmapskia/MapPainterSkia.h` with class inheriting `MapPainter`, declaring all overrides, and defining `SkiaLabel`/`SkiaGlyph` types (following AGG/Cairo pattern)
- [x] 4.2 Declare `DrawMap()` public method accepting `SkCanvas*` with `RenderSteps` start/end parameters
- [x] 4.3 Declare private members: `SkCanvas* draw`, `std::mutex mutex`, label layouter

## 5. MapPainterSkia Implementation

- [x] 5.1 Create `src/osmscoutmapskia/MapPainterSkia.cpp` with constructor, destructor, and `DrawMap()` method (initialize Skia state, call `Draw()`, cleanup)
- [x] 5.2 Implement `DrawGround()` — fill entire canvas with `FillStyle` color using `SkCanvas::drawColor()` or `SkCanvas::drawRect()`
- [x] 5.3 Implement `DrawArea()` — build `SkPath` from `AreaData::coordRange`, fill with `FillStyle` color using `SkCanvas::drawPath()`
- [x] 5.4 Implement `DrawPath()` — build `SkPath` from `CoordBufferRange`, stroke with `Color`/width using `SkCanvas::drawPath()`
- [x] 5.5 Implement `HasIcon()` — return `false`
- [x] 5.6 Implement `GetFontHeight()` — return `12.0`
- [x] 5.7 Implement `RegisterRegularLabel()` — no-op
- [x] 5.8 Implement `RegisterContourLabel()` — no-op
- [x] 5.9 Implement `DrawLabels()` — no-op
- [x] 5.10 Implement `DrawIcon()` — no-op
- [x] 5.11 Implement `DrawSymbol()` — no-op
- [x] 5.12 Implement `DrawContourSymbol()` — no-op
- [x] 5.13 Verify `MapPainterSkia.cpp` compiles without errors

## 6. Demo Application

- [x] 6.1 Create `Demos/src/DrawMapSkia.cpp` — follow `DrawMapAgg.cpp` pattern but use `SkSurface::Raster()` and PPM output
- [x] 6.2 Add `DrawMapSkia` target in `Demos/CMakeLists.txt` (guarded by `OSMSCOUT_BUILD_MAP_SKIA`)
- [x] 6.3 Add `DrawMapSkia` target in `Demos/meson.build` (guarded by `buildMapSkia`)
- [x] 6.4 Add `HAVE_LIB_OSMSCOUTMAPSKIA` to `Demos/src/meson.build` config data
- [x] 6.5 Verify `DrawMapSkia` compiles and links

## 7. DrawMapAll Multi-Backend Integration

- [x] 7.1 Add `#include <osmscoutmapskia/MapPainterSkia.h>` in `Demos/src/DrawMapAll.cpp` (guarded by `HAVE_OSMSCOUT_MAP_SKIA`)
- [x] 7.2 Add Skia rendering block in `DrawMapAll.cpp` main function
- [x] 7.3 Add `OSMScout::MapSkia` to `DRAWMAPALL_TARGETS` and `HAVE_OSMSCOUT_MAP_SKIA` to `DRAWMAPALL_DEFS` in `Demos/CMakeLists.txt`
- [x] 7.4 Add Skia conditional block in `Demos/meson.build` `DrawMapAll` section
- [x] 7.5 Verify `DrawMapAll` compiles with Skia backend enabled

## 8. Compilation Test

- [x] 8.1 Create `Tests/src/MapPainterSkiaTest.cpp` with a Catch2 test that instantiates `MapPainterSkia` and verifies it compiles and links
- [x] 8.2 Add test target in `Tests/CMakeLists.txt` and `Tests/meson.build` (guarded by Skia backend availability)
- [x] 8.3 Verify test compiles and passes

## 9. Verification

- [x] 9.1 Build entire project with CMake (`-DOSMSCOUT_BUILD_MAP_SKIA=ON`) — no new warnings or errors
- [x] 9.2 Build entire project with Meson (`-DenableMapSkia=true`) — no new warnings or errors
- [x] 9.3 Run existing test suite — no regressions
- [x] 9.4 Run `DrawMapSkia` demo with a real map database and verify PNG output is produced
