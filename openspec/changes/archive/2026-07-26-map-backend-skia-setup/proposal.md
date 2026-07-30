## Why

libosmscout needs a Skia-based map rendering backend. Skia is a mature 2D graphics library used by Chrome, Android, and Flutter — it offers hardware-accelerated rendering, modern text layout, and cross-platform support. Adding a Skia backend gives libosmscout a modern, performant rendering option alongside existing backends (AGG, Cairo, OpenGL, Qt).

## What Changes

- New `libosmscout-map-skia/` subproject with full build integration
- New `MapPainterSkia` class implementing the `MapPainter` interface
- New `MapSkiaImportExport.h` and `MapSkiaFeatures.h.cmake` for export macros and feature detection
- New `DrawMapSkia` demo application
- Skia backend registration in `DrawMapAll` multi-backend demo
- Build system changes: CMake option `OSMSCOUT_BUILD_MAP_SKIA`, Meson option `enableMapSkia`
- Skia dependency detection in CMake (`features.cmake`) and Meson (`meson.build`)
- Initial minimal rendering: `DrawGround`, `DrawArea` with solid fill, `DrawPath` with solid stroke
- Stub implementations for remaining pure virtual methods (returning true/no-op) to get compilation
- Test infrastructure: basic compilation test

## Capabilities

### New Capabilities
- `skia-renderer`: Skia-based `MapPainterSkia` class implementing the full `MapPainter` interface, with initial support for area fills, path strokes, ground rendering, and label layout scaffolding
- `skia-build`: CMake and Meson build integration for the Skia backend, including dependency detection, option flags, and conditional compilation
- `skia-demo`: `DrawMapSkia` demo application and `DrawMapAll` integration for testing the rendering pipeline end-to-end

### Modified Capabilities
- (none — new backend, no existing spec changes)

## Impact

- **New directory**: `libosmscout-map-skia/` with `include/osmscoutmapskia/`, `src/osmscoutmapskia/`
- **Build system**: root `CMakeLists.txt`, root `meson.build`, `meson_options.txt`, `cmake/features.cmake`
- **Demos**: `Demos/CMakeLists.txt`, `Demos/meson.build`, `Demos/src/meson.build` (config.h generation)
- **Dependencies**: Skia library (system or bundled via subproject wrap)
- **Platforms**: Linux, macOS, Windows (Skia is cross-platform)
