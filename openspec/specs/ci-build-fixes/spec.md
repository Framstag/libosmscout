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

### Requirement: Required build dependencies are provided by every entry point that builds the component

Every CI entry point that configures or builds a component SHALL provide that component's required
build dependencies before configuring it. A configuration that deliberately does not build a
component SHALL NOT be required to provide that component's dependencies, and SHALL be documented as
exempt rather than left out. When a required build dependency cannot be satisfied, the run SHALL fail
before any release artifact is published and SHALL report which dependency is missing.

#### Scenario: Release workflows provide the import library's JSON dependency

- **GIVEN** the import library declares a required JSON implementation
- **WHEN** the dependency installation step of a release workflow is inspected
- **THEN** the JSON implementation SHALL be installed by that workflow before the project is
  configured

#### Scenario: Every release entry point is covered

- **GIVEN** more than one workflow can create a release
- **WHEN** each release workflow's dependency installation step is inspected
- **THEN** every release workflow that configures the project SHALL install the required JSON
  implementation, and no release workflow SHALL rely on another workflow's installation step

#### Scenario: Build and test workflows already provide it

- **GIVEN** a workflow that builds the import library for pull-request validation
- **WHEN** its dependency installation step is inspected
- **THEN** the required JSON implementation SHALL be installed by that workflow

#### Scenario: A configuration that does not build the import library is exempt

- **GIVEN** a build configuration that disables the import library
- **WHEN** that configuration is configured without the JSON implementation present
- **THEN** configuration SHALL succeed without the JSON implementation being required

#### Scenario: An unsatisfied required dependency stops the release

- **GIVEN** a release workflow whose environment lacks a dependency the configured project requires
- **WHEN** the workflow runs
- **THEN** the run SHALL end in failure before any release artifact is published
- **AND** the run SHALL report the name of the missing dependency

### Requirement: A dependency the backends feature-gate is not a configure requirement

A configure of the project SHALL complete when a dependency that the map backends feature-gate is
disabled on the command line, and the corresponding feature gate SHALL then be off in the configured
feature header of the backend that consumes it. Disabling such a dependency SHALL NOT abort the
configure, and a configure that leaves it enabled SHALL still find it and define the gate.

#### Scenario: A configure with the font configuration disabled completes

- **GIVEN** a host whose font configuration is found by the project
- **WHEN** `cmake -B <dir> -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Fontconfig=ON` is run
- **THEN** the configure SHALL reach `-- Configuring done` and exit 0
- **AND** `OSMSCOUT_MAP_CAIRO_HAVE_LIB_FONTCONFIG` SHALL be undefined in the configured
  `libosmscout-map-cairo/include/osmscoutmapcairo/MapCairoFeatures.h`

#### Scenario: A configure with FreeType disabled completes

- **GIVEN** a host whose FreeType is found by the project and whose OpenGL map backend is disabled
- **WHEN** `cmake -B <dir> -DCMAKE_BUILD_TYPE=Release -DOSMSCOUT_BUILD_MAP_OPENGL=OFF -DCMAKE_DISABLE_FIND_PACKAGE_Freetype=ON` is run
- **THEN** the configure SHALL reach `-- Configuring done` and exit 0
- **AND** `OSMSCOUT_MAP_CAIRO_HAVE_LIB_FREETYPE` and `OSMSCOUT_MAP_CAIRO_HAVE_LIB_CAIRO_FT` SHALL be
  undefined in the configured feature header

#### Scenario: A configure that leaves both enabled still finds both

- **GIVEN** a host that provides FreeType and the font configuration
- **WHEN** `cmake -B <dir> -DCMAKE_BUILD_TYPE=Release` is run without disabling either
- **THEN** the configure SHALL exit 0, report `Found Freetype` and `Found Fontconfig`, and define
  `OSMSCOUT_MAP_CAIRO_HAVE_LIB_FREETYPE` and `OSMSCOUT_MAP_CAIRO_HAVE_LIB_FONTCONFIG` in the configured
  feature header

#### Scenario: The Cairo backend compiles with either dependency disabled

- **GIVEN** a build directory configured as in the two disabled scenarios above
- **WHEN** `cmake --build <dir> --target OSMScoutMapCairo` is run
- **THEN** the target SHALL build and link without an error or warning
