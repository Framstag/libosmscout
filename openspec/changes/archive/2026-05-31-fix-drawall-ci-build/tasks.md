## 1. Restore Srtm demo in CMakeLists.txt

- [x] 1.1 Add `osmscout_demo_project(NAME Srtm SOURCES src/Srtm.cpp TARGET OSMScout::OSMScout)` after the DrawMapAll block in `Demos/CMakeLists.txt`

## 2. Guard libpng dependency in DrawMapAll.cpp

- [x] 2.1 Guard `#include <png.h>` with `#if defined(HAVE_OSMSCOUT_MAP_AGG) || defined(HAVE_OSMSCOUT_MAP_OPENGL)` / `#endif`
- [x] 2.2 Guard `WriteRGBToPNG()` and `BackendPath()` helper functions with same `#if` condition
- [x] 2.3 Guard the AGG rendering block's `WriteRGBToPNG()` call (already inside `#if defined(HAVE_OSMSCOUT_MAP_AGG)`) — ensure `BackendPath()` usage is also guarded

## 3. Fix MSYS/MinGW compilation errors

- [x] 3.1 Guard `#define NOMINMAX` with `#ifndef NOMINMAX` / `#define NOMINMAX` / `#endif`
- [x] 3.2 Fix `wpath.c()` → `wpath.c_str()` in DirectX backend section (line ~524)

## 4. Fix macOS meson build

- [x] 4.1 Replace hardcoded `'../build/libosmscout-map-iosx/include'` with `osmscoutmapiosxIncDir` in `Demos/meson.build`
- [x] 4.2 Verify `osmscoutmapiosxIncDir` variable is defined (check `libosmscout-map-iosx/meson.build`)

## 5. Fix macOS cmake build

- [x] 5.1 Investigate and fix `DrawMapAllOSX.mm:13` compile error — `include_directories(DrawMapAll PRIVATE ...)` is invalid CMake (takes no target arg), fixed to `target_include_directories`

## 6. Install libpng-dev in all CI workflows

- [x] 6.1 Add `libpng-dev` to all 5 `apt-get install` lists in `.github/workflows/build_and_test_on_ubuntu_24_04.yml`
- [x] 6.2 Add `libpng` to both `brew install` lines in `.github/workflows/build_and_test_on_osx.yml`
- [x] 6.3 Add `mingw-w64-x86_64-libpng` to both MSYS install lists in `.github/workflows/build_and_test_on_msys.yml`
