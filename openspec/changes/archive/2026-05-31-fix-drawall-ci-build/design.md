## Context

Commit c14a0e0bf added DrawMapAll — a multi-backend render demo that outputs PNG images via any available map backend (AGG, Cairo, OpenGL, Qt, SVG, DirectX, GDI, OSX). It built on the author's local machine but broke CI across multiple platforms:

| CI Build | Failure |
|----------|---------|
| Ubuntu cmake Qt6 | `#include <png.h>` — no libpng in Qt6-only env |
| macOS cmake | `DrawMapAllOSX.mm` compile error |
| macOS meson | `../build/libosmscout-map-iosx/include` — hardcoded build dir doesn't exist during `meson setup` |
| MSYS meson | 3 failures: (1) `png.h` not found, (2) `NOMINMAX` redefined, (3) `wpath.c()` should be `c_str()` |
| Windows VS meson | `png.h` not found |
| All CMake builds | Srtm demo project definition was accidentally deleted |

Root cause: `#include <png.h>` is unconditional in `DrawMapAll.cpp`, but only AGG and OpenGL backends use libpng. Other backends (Cairo, Qt, SVG etc.) write PNG via their own APIs. CI images don't all install libpng-dev.

## Goals / Non-Goals

**Goals:**
- Restore Srtm demo in CMakeLists.txt
- Guard `#include <png.h>` so it only compiles when AGG or OpenGL is active
- Fix `wpath.c()` → `wpath.c_str()` (typo)
- Fix `NOMINMAX` redefinition (MSYS)
- Fix macOS meson build: replace hardcoded build dir path with variable
- Fix macOS cmake DrawMapAllOSX.mm error
- Ensure `WriteRGBToPNG` helper doesn't produce unused-function warning when no PNG backend active

**Non-Goals:**
- No architectural changes to DrawMapAll
- No changes to backend-specific rendering logic

## Decisions

### Decision 1: Guard `#include <png.h>` with backend-specific defines

**Chosen:** Guard with `#if defined(HAVE_OSMSCOUT_MAP_AGG) || defined(HAVE_OSMSCOUT_MAP_OPENGL)` — the two backends that use the raw-RGB→PNG path.

**Rationale:** Only AGG and OpenGL write raw RGB buffers and need `WriteRGBToPNG()`. All other backends (Cairo, Qt, SVG, DirectX, GDI, OSX) write PNG via their own library APIs. The `HAVE_OSMSCOUT_MAP_*` defines are already set per-backend by both CMake and Meson.

**Alternative considered:** Add a `DRAWMAPALL_NEEDS_PNG` compile definition — but that would require modifications to both build systems. Using existing defines is simpler and consistent with how the rest of the file works.

### Decision 2: Guard `WriteRGBToPNG()` and `BackendPath()` with same condition

**Chosen:** Wrap the entire AGG + OpenGL rendering blocks (including the helper function) in the same `#if` guard. The `BackendPath()` helper is also only used inside these blocks.

**Rationale:** Prevents "unused function" warnings on builds where only Cairo/Qt/SVG/DirectX/GDI backends are active (e.g. MSYS without AGG).

### Decision 3: Fix macOS meson: use variable instead of hardcoded path

**Chosen:** Use `osmscoutmapiosxIncDir` variable (consistent with all other backends) instead of hardcoded `../build/libosmscout-map-iosx/include`.

**Rationale:** The meson `libosmscout-map-iosx` subproject already exposes `osmscoutmapiosxIncDir`. All other backends reference their `*IncDir` variable. The hardcoded path was a copy-paste from the CMake build system and doesn't work during `meson setup` (build dir may not exist yet).

### Decision 4: Guard `NOMINMAX` with `#ifndef`

**Chosen:** `#ifndef NOMINMAX` / `#define NOMINMAX` / `#endif` on the existing `#define NOMINMAX` at line 70.

**Rationale:** MSYS/C++ headers now define `NOMINMAX` themselves. Guarding prevents redefinition warning. This is standard practice across the codebase.

### Decision 5: Fix `wpath.c()` → `wpath.c_str()`

**Chosen:** Change line 524 from `wpath.c()` to `wpath.c_str()`.

**Rationale:** `std::wstring` has no `.c()` method — this was a typo. `InitializeFromFilename` expects `const wchar_t*`.

## Decisions

### Decision 6: Install libpng-dev in all CI workflows

**Chosen:** Add `libpng-dev` to apt-get, `libpng` to brew, and `mingw-w64-x86_64-libpng` to MSYS package lists in every CI workflow that builds demos.

**Rationale:** Rather than relying solely on conditional compilation guards, ensuring libpng is always available makes the build more robust and consistent. The `find_package(PNG)` call is already optional in CMake, but having the library present means the code guard and the build system guard (`DRAWMAPALL_NEEDS_PNG` / `drawMapAllNeedPng`) can remain simple. This is the path of least surprise — libpng is a small, stable, universally-packaged dependency.

**Alternative considered:** Making `PNG::PNG` REQUIRED-only when AGG/OpenGL are enabled — but that's more complex build logic with no real benefit over simply installing the package everywhere.

### Decision 7: Keep `#include <png.h>` guard even with libpng installed everywhere

**Chosen:** Still guard `#include <png.h>` behind `HAVE_OSMSCOUT_MAP_AGG || HAVE_OSMSCOUT_MAP_OPENGL` even though all CI will have libpng.

**Rationale:** Belt-and-suspenders. Local dev builds without libpng should still work. The guard costs nothing.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Another backend added that needs raw RGB→PNG | Developer must add its `HAVE_` define to the guard condition |
| macOS cmake `DrawMapAllOSX.mm` error may have deeper cause | Investigate after guard fixes are applied; error may be a side effect of other issues |
| Meson `osmscoutmapiosxIncDir` may not exist if iOSX subproject is optional | Already checked — `buildMapIOSX` is the guard, and the inc dir is only referenced inside that block |
| CI workflow can be forgotten when adding new jobs | Standard practice: all workflows that build demos install the same dependency set

## Open Questions

1. Exact error in macOS cmake `DrawMapAllOSX.mm:13` — need to see full build log to determine cause. Likely an include path issue resolved by the broader fix round.
