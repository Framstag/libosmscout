## ADDED Requirements

### Requirement: CMake build option
The build system SHALL provide a `OSMSCOUT_BUILD_MAP_SKIA` CMake option to enable or disable the Skia backend.

#### Scenario: Option defaults to ON when Skia found
- **WHEN** Skia library is detected by CMake and `OSMSCOUT_BUILD_MAP` is ON
- **THEN** `OSMSCOUT_BUILD_MAP_SKIA` defaults to ON

#### Scenario: Option can be explicitly disabled
- **WHEN** user sets `-DOSMSCOUT_BUILD_MAP_SKIA=OFF`
- **THEN** the Skia backend is not built regardless of Skia availability

### Requirement: CMake Skia detection
The build system SHALL detect the Skia library using `find_package(Skia)` or a custom `FindSkia.cmake` module.

#### Scenario: Skia found via pkg-config
- **WHEN** `pkg-config` provides `skia` and the header `SkCanvas.h` is found
- **THEN** `Skia_FOUND` is TRUE and `Skia::skia` target is available

#### Scenario: Skia not found produces warning
- **WHEN** Skia is not found and `OSMSCOUT_BUILD_MAP_SKIA` is ON
- **THEN** a status message "Preconditions for building libosmscout-map-skia MISSING" is printed and the backend is skipped

### Requirement: CMake subdirectory registration
The root `CMakeLists.txt` SHALL conditionally add `libosmscout-map-skia/` as a subdirectory when the backend is enabled.

#### Scenario: Subdirectory added when enabled
- **WHEN** `OSMSCOUT_BUILD_MAP_SKIA` is ON
- **THEN** `add_subdirectory(libosmscout-map-skia)` is called

### Requirement: CMake library target
The `libosmscout-map-skia/CMakeLists.txt` SHALL define a library target `OSMScoutMapSkia` with alias `OSMScout::MapSkia` and output name `osmscout_map_skia`.

#### Scenario: Library target exists
- **WHEN** building with `OSMSCOUT_BUILD_MAP_SKIA=ON`
- **THEN** target `OSMScoutMapSkia` exists and links against `OSMScout::OSMScout`, `OSMScout::Map`, and `Skia::skia`

### Requirement: Meson build option
The build system SHALL provide an `enableMapSkia` Meson option.

#### Scenario: Option defined in meson_options.txt
- **WHEN** inspecting `meson_options.txt`
- **THEN** `option('enableMapSkia', type: 'boolean', value: true, description: 'Build Skia backend')` is present

### Requirement: Meson Skia dependency detection
The Meson build SHALL detect Skia via `dependency('skia', required: false)` with a subproject wrap fallback.

#### Scenario: Skia detected via system
- **WHEN** `skia` is available on the system
- **THEN** `skiaDep.found()` is true and the backend is conditionally built

#### Scenario: Subproject wrap fallback
- **WHEN** Skia is not on the system but `subprojects/skia.wrap` exists
- **THEN** the wrap is used as fallback

#### Scenario: Manual fallback when no pkg-config
- **WHEN** Skia is not found via pkg-config or cmake
- **THEN** the build system falls back to checking for `core/SkCanvas.h` header and `libskia` directly

### Requirement: Meson subdirectory registration
The root `meson.build` SHALL conditionally add `libosmscout-map-skia/` as a subdirectory.

#### Scenario: Subdirectory added when enabled
- **WHEN** `buildMapSkia` is true
- **THEN** `subdir('libosmscout-map-skia')` is called

### Requirement: Meson library target
The `libosmscout-map-skia/meson.build` SHALL define a library `osmscout_map_skia` linking against `osmscoutmap`, `osmscout`, and Skia.

#### Scenario: Library builds with Meson
- **WHEN** building with `-DenableMapSkia=true` and Skia available
- **THEN** `libosmscout_map_skia` is built and installed

### Requirement: Feature detection header
The build system SHALL generate `MapSkiaFeatures.h` from `MapSkiaFeatures.h.cmake` / `MapSkiaFeatures.h.meson` to expose build-time feature flags.

#### Scenario: Header generated
- **WHEN** configuring the build
- **THEN** `MapSkiaFeatures.h` is generated in the build directory with include guard `LIBOSMSCOUT_MAP_SKIA_FEATURES`
