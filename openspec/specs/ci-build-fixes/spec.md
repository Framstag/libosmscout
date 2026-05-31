# CI Build Fixes

## Purpose

Ensure the DrawMapAll multi-backend render demo compiles across all CI platforms by guarding platform-specific code and ensuring all required dependencies are installed. This spec captures the build hardening applied after the initial DrawMapAll commit.

## Requirements

### Requirement: DrawMapAll compiles without libpng when not needed

The `#include <png.h>` directive SHALL only be compiled when either the AGG or OpenGL backend is enabled. On builds with only Cairo/Qt/SVG/DirectX/GDI backends, no libpng dependency SHALL be required.

#### Scenario: Build with Cairo+Qt+SVG only compiles without libpng
- **WHEN** CMake is configured with Cairo, Qt, and SVG map backends enabled but AGG and OpenGL disabled
- **AND** libpng is not installed on the system
- **THEN** the build SHALL succeed without errors

#### Scenario: Build with AGG backend links libpng
- **WHEN** CMake is configured with AGG map backend enabled
- **THEN** `PNG::PNG` SHALL be linked to DrawMapAll
- **AND** `#include <png.h>` SHALL be compiled

#### Scenario: Build with OpenGL backend links libpng
- **WHEN** CMake is configured with OpenGL map backend enabled
- **THEN** `PNG::PNG` SHALL be linked to DrawMapAll
- **AND** `#include <png.h>` SHALL be compiled

### Requirement: WriteRGBToPNG helper is guarded

The `WriteRGBToPNG()` helper functions SHALL only be compiled when AGG or OpenGL backend is enabled, to avoid "unused function" warnings.

#### Scenario: No unused-function warning on Cairo-only build
- **WHEN** the build has Cairo backend but no AGG or OpenGL
- **THEN** the compiler SHALL NOT emit a warning about `WriteRGBToPNG` being unused

### Requirement: NOMINMAX redefinition is fixed

The `#define NOMINMAX` in DrawMapAll.cpp SHALL use include guards to prevent redefinition warnings on MSYS/MinGW.

#### Scenario: MSYS build with NOMINMAX defined in system headers
- **WHEN** building on MSYS/MinGW where system headers already define `NOMINMAX`
- **THEN** no redefinition warning SHALL be emitted for `NOMINMAX`

### Requirement: wpath.c() typo fixed

The `std::wstring` method call `.c()` on the DirectX backend path SHALL be corrected to `.c_str()`.

#### Scenario: DirectX backend compiles on MSVC
- **WHEN** building the DirectX backend path on MSVC/Windows
- **THEN** `stream->InitializeFromFilename(wpath.c_str(), GENERIC_WRITE)` SHALL compile without error

### Requirement: Srtm demo is restored in CMakeLists.txt

The Srtm demo project definition SHALL be present in `Demos/CMakeLists.txt` after the DrawMapAll block.

#### Scenario: CMakeLists.txt contains Srtm project
- **WHEN** viewing the end of `Demos/CMakeLists.txt`
- **THEN** there SHALL be a line `osmscout_demo_project(NAME Srtm SOURCES src/Srtm.cpp TARGET OSMScout::OSMScout)`

### Requirement: macOS meson build uses variable include path

The macOS/iOSX backend include directory in `Demos/meson.build` SHALL use the meson variable `osmscoutmapiosxIncDir` instead of a hardcoded `../build/libosmscout-map-iosx/include` path.

#### Scenario: Meson setup on macOS succeeds
- **WHEN** running `meson setup` on macOS with iOSX backend
- **THEN** the include directory for iOSX SHALL resolve without requiring a pre-existing build directory

### Requirement: CI workflows install libpng

Every CI workflow that builds demos SHALL install libpng (`libpng-dev` on Ubuntu, `libpng` on macOS, `mingw-w64-x86_64-libpng` on MSYS) so that `PNG::PNG` is always available.

#### Scenario: Ubuntu CI apt-get includes libpng-dev
- **WHEN** inspecting the `apt-get install` commands in `.github/workflows/build_and_test_on_ubuntu_24_04.yml`
- **THEN** `libpng-dev` SHALL appear in every package list for jobs that build demos

#### Scenario: macOS CI brew includes libpng
- **WHEN** inspecting the `brew install` command in `.github/workflows/build_and_test_on_osx.yml`
- **THEN** `libpng` SHALL appear in the package list

#### Scenario: MSYS CI includes mingw-w64-x86_64-libpng
- **WHEN** inspecting the `setup-msys2` install list in `.github/workflows/build_and_test_on_msys.yml`
- **THEN** `mingw-w64-x86_64-libpng` SHALL appear in the package list
