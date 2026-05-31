## Why

Commit c14a0e0bf ("feat: issue #960") added DrawMapAll but accidentally deleted the Srtm demo project definition from `Demos/CMakeLists.txt`. It also introduced `#include <png.h>` and `target_link_libraries(DrawMapAll PNG::PNG)` which will fail on CI builds where libpng is unavailable or `find_package(PNG)` doesn't produce a valid `PNG::PNG` target. The meson build (`Demos/meson.build`) needs a matching PNG dependency guard.

## What Changes

- **Restore Srtm demo**: The `osmscout_demo_project(NAME Srtm ...)` line was dropped when DrawMapAll was inserted. Re-add it below the DrawMapAll block.
- **Guard PNG dependency**: `target_link_libraries(DrawMapAll PNG::PNG)` is called only when `DRAWMAPALL_NEEDS_PNG` is ON (AGG or OpenGL backends active). This is correct, but the unconditional `#include <png.h>` in `DrawMapAll.cpp` will break builds without libpng. Guard the include with `#ifdef DRAWMAPALL_NEEDS_PNG` or a compile definition.
- **Meson build**: Add `pngDep` to `drawMapAllDep` only when `drawMapAllNeedPng` is true (already done in meson.build but verify the guard condition matches CMake logic).
- **Install libpng-dev in all CI workflows**: Ensure `libpng-dev` (Ubuntu), `libpng` (macOS brew), and `mingw-w64-x86_64-libpng` (MSYS) are installed in every CI workflow that builds demos, so `PNG::PNG` is always available.

## Capabilities

### New Capabilities
- `ci-build-fixes`: fixes to restore CI builds broken by the DrawMapAll addition

### Modified Capabilities
*(none — fixing existing code, no spec-level behavior changes)*

## Impact

- `Demos/CMakeLists.txt` — restore Srtm, add PNG include guard
- `Demos/src/DrawMapAll.cpp` — guard `#include <png.h>`
- `Demos/meson.build` — verify PNG guard logic matches CMake
- `.github/workflows/build_and_test_on_ubuntu_24_04.yml` — add `libpng-dev` to apt-get lists
- `.github/workflows/build_and_test_on_osx.yml` — add `libpng` to brew install
- `.github/workflows/build_and_test_on_msys.yml` — add `mingw-w64-x86_64-libpng` to MSYS install
- `.github/workflows/build_and_test_on_vs2025.yml` — libpng comes via vcpkg wrapper, no change needed
- All CI workflows that build demos will pass again
