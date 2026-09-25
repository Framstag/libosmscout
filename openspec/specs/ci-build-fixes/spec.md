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

### Requirement: MSYS jobs provide the font the font-dependent tests measure against

The MSYS CI jobs that run the font-dependent tests (`MapPainterShieldTest`, `PerformanceTest-cairo-*`) SHALL make the font family those tests name resolvable to the font file the repository ships, so that the measured glyph metrics do not depend on the fonts the runner image or the MSYS2 package set happen to provide.

#### Scenario: Named family resolves to the repository font

- **WHEN** the font-dependent tests run in the MSYS job's test environment
- **THEN** the font family they name SHALL resolve to the repository's Liberation Sans font file
- **AND** the tests SHALL NOT measure a fallback font

#### Scenario: Provisioning survives a runner image or package change

- **WHEN** a runner image update or an MSYS2 package update changes the set of fonts the environment provides
- **THEN** the family the tests name SHALL still resolve to the repository's font file
- **AND** the font-dependent tests SHALL produce the same metric results as before the update

### Requirement: MSYS jobs verify the font and locale environment before running tests

Each MSYS job SHALL verify, in a step of its own before the test step, that the font family the font-dependent tests name resolves to the repository font file and that a locale is in effect, and SHALL fail that step when either check does not hold.

#### Scenario: Broken font environment fails before the tests run

- **GIVEN** an environment in which the named font family resolves to a fallback instead of the repository font file
- **WHEN** the MSYS job reaches the verification step
- **THEN** the step SHALL fail
- **AND** the test step SHALL NOT run
- **AND** the job output SHALL name the font the family resolved to

#### Scenario: Invalid locale is reported

- **GIVEN** an environment in which the locale in effect is not valid
- **WHEN** the MSYS job reaches the verification step
- **THEN** the job output SHALL name the locale in effect

#### Scenario: Verification passes on a provisioned environment

- **GIVEN** a font environment in which the named family resolves to the repository font file
- **WHEN** the MSYS job reaches the verification step
- **THEN** the step SHALL pass
- **AND** the test step SHALL run

### Requirement: MSYS test steps run with an explicit locale

Both MSYS jobs SHALL run their test steps with an explicitly set locale instead of inheriting the locale the MSYS2 environment provides, and both jobs SHALL use the same locale setting.

#### Scenario: Environment locale is valid in the cmake job

- **WHEN** the `gcc and cmake` job runs the tests in `.github/workflows/build_and test_on_msys.yml` with `-DHAVE_LOCALE` code paths active
- **THEN** the environment locale SHALL be valid
- **AND** no test SHALL log that it failed to obtain the environment locale

#### Scenario: Both MSYS jobs use the same locale

- **WHEN** inspecting the test steps of the `gcc and cmake` and `gcc and meson` jobs in `.github/workflows/build_and test_on_msys.yml`
- **THEN** both test steps SHALL declare the same locale value

### Requirement: MSYS font provisioning uses only repository content and the existing package setup

The MSYS font provisioning SHALL use the font file already in the repository and SHALL NOT require a font package that MSYS2 does not provide, a network download beyond the package installation the jobs already perform, or an additional secret.

#### Scenario: No new external dependency

- **WHEN** inspecting the MSYS job setup in `.github/workflows/build_and test_on_msys.yml`
- **THEN** the provisioned font SHALL originate from a file in the repository
- **AND** no additional secret or service SHALL be required
