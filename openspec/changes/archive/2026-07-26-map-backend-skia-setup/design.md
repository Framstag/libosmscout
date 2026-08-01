## Context

libosmscout currently has 8 map rendering backends (AGG, Cairo, OpenGL, Qt, SVG, DirectX, GDI, iOSX). Each backend lives in its own `libosmscout-map-{name}/` subproject with a `MapPainter{Name}` class inheriting from `osmscout::MapPainter`. The base class defines ~12 pure virtual methods for drawing primitives (areas, paths, symbols, icons, labels, ground) plus optional lifecycle callbacks.

Skia is a 2D graphics library maintained by Google, used in Chrome, Android, Flutter, and Firefox. It provides hardware-accelerated rendering via GPU backends (Vulkan, Metal, OpenGL) and a software rasterizer. It has C API bindings (`skia.h`) and a C++ API (`SkCanvas`, `SkPaint`, `SkPath`, etc.).

This design covers the initial boilerplate setup — directory structure, build integration, empty/stub `MapPainterSkia` class, demo app, and minimal rendering to verify the call chain.

## Goals / Non-Goals

**Goals:**
- New `libosmscout-map-skia/` subproject with CMake + Meson build files
- Skia dependency detection in both build systems
- `MapPainterSkia` class inheriting `MapPainter` with all pure virtual methods implemented (stubs + minimal real rendering)
- `MapSkiaImportExport.h` for DLL export macros
- `MapSkiaFeatures.h.cmake` for feature detection
- `DrawMapSkia` standalone demo app rendering to PNG
- Skia backend registered in `DrawMapAll` multi-backend demo
- Build option `OSMSCOUT_BUILD_MAP_SKIA` (CMake) / `enableMapSkia` (Meson)
- Minimal rendering: `DrawGround` (fill canvas), `DrawArea` (solid fill polygon), `DrawPath` (solid stroke)
- Compilation test in `Tests/`

**Non-Goals:**
- Full feature-complete rendering (labels, icons, symbols, patterns, dashes, contour labels, hill shading, etc.) — these get real implementations in follow-up changes
- Text/font rendering — `GetFontHeight` returns a constant, label methods are no-ops
- GPU-accelerated rendering — initial version uses Skia software rasterizer only
- Performance optimization
- Android/iOS platform support (follows later)

## Decisions

### Decision 1: Skia dependency via `find_package` / `dependency()` with fallback to subproject wrap

**Chosen:** Use `pkg-config` / `find_package` for system Skia, with Meson subproject wrap as fallback.

**Alternatives considered:**
1. **Bundle Skia source** — Large (10MB+), complex build. Skia uses its own GN build system, hard to integrate.
2. **vcpkg/Conan only** — Excludes users who have system Skia or want a subproject wrap.
3. **Always require system Skia** — Skia is not widely packaged; many distros lack it.

**Rationale:** Meson wraps (`subprojects/skia.wrap`) can download prebuilt Skia or build from source. CMake can use `FindSkia.cmake` module. This matches the pattern used for other deps (libxml2, protobuf, glew).

### Decision 2: Directory structure mirrors existing backends

**Chosen:** Follow `libosmscout-map-agg` layout exactly.

```
libosmscout-map-skia/
  CMakeLists.txt
  meson.build
  include/
    meson.build
    osmscoutmapskia/
      MapSkiaImportExport.h
      MapSkiaFeatures.h.cmake
      MapPainterSkia.h
  src/
    meson.build
    osmscoutmapskia/
      MapPainterSkia.cpp
```

**Alternatives considered:**
1. **Flat structure** — Inconsistent with project conventions.
2. **Single header/source** — Would work but breaks the established pattern.

**Rationale:** Consistency with all 8 existing backends makes maintenance easier and matches CI expectations.

### Decision 3: `MapPainterSkia` uses `SkCanvas*` as the drawing target

**Chosen:** `DrawMap()` accepts `SkCanvas*` (similar to Cairo's `cairo_t*`).

**Alternatives considered:**
1. **`SkSurface*`** — More ownership, less flexible for embedding.
2. **`SkBitmap*`** — Software-only, no GPU path.
3. **`SkCanvas*` + `SkSurface*` pair** — Redundant; canvas is the standard drawing API entry point.

**Rationale:** `SkCanvas` is Skia's primary drawing interface. The demo app creates an `SkSurface` from a raster bitmap and passes the canvas. This matches the Cairo pattern (`cairo_t*`).

### Decision 4: Stub implementations for non-goal methods

**Chosen:** Pure virtual methods not in scope get minimal stubs:
- `HasIcon()` → returns `false` (no icons loaded)
- `GetFontHeight()` → returns `12.0` (constant)
- `RegisterRegularLabel()` / `RegisterContourLabel()` → no-op
- `DrawLabels()` → no-op
- `DrawIcon()` → no-op
- `DrawSymbol()` → no-op
- `DrawContourSymbol()` → no-op

**Alternatives considered:**
1. **Throw/assert** — Would crash on any map with labels/icons.
2. **Full implementation** — Out of scope for this change.

**Rationale:** Stubs let the backend compile and render basic maps (areas, paths, ground). Users can test the pipeline end-to-end. Real implementations come in follow-up changes.

### Decision 5: Demo app renders to PNG via Skia's `SkFILEWStream` (with PPM fallback)

**Chosen:** `DrawMapSkia` writes PNG output using `SkPngEncoder` when available. Falls back to PPM output (matching the AGG demo pattern) when the system Skia installation lacks PNG encoding support.

**Alternatives considered:**
1. **PPM output** — Simpler but less useful. AGG demo uses PPM. Used as fallback.
2. **Skia `SkDebugCanvas`** — Debug-only, not for end users.
3. **Display window** — Requires GLFW/SDL, adds dependency.

**Rationale:** PNG is universally viewable. Skia's built-in `SkPngEncoder` needs no extra deps. The Cairo demo also writes PNG. PPM fallback ensures the demo works even on systems with incomplete Skia installations.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Skia not available on target system | Meson subproject wrap provides fallback; CMake `FindSkia.cmake` module with clear error message |
| Skia API changes between versions | Pin minimum version in build config; use stable C++ API subset |
| Stub methods produce invisible rendering for complex maps | Demo app uses simple stylesheet; user gets visual feedback for areas/paths |
| Large binary size from Skia linkage | Skia supports symbol stripping; shared library build reduces per-backend cost |
| Build time increase from Skia compilation (if built from source) | Subproject wrap can use prebuilt binaries; CI caches dependencies |
