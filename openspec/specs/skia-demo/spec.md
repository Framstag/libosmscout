# Skia Demo

## Purpose

Provide demo applications for testing the Skia map rendering backend end-to-end, including a standalone `DrawMapSkia` executable and integration with the multi-backend `DrawMapAll` demo.

## Requirements

### Requirement: DrawMapSkia demo application
The system SHALL provide a `DrawMapSkia` executable demo that renders a map using the Skia backend and writes a PNG output file.

#### Scenario: DrawMapSkia produces PNG
- **WHEN** running `DrawMapSkia <stylesheet> <lat> <lon> <zoom> <output.png>` with valid arguments including `--fontName` and `--database`
- **THEN** a PNG file is written to the specified output path containing the rendered map

#### Scenario: DrawMapSkia accepts standard demo arguments
- **WHEN** running `DrawMapSkia --help`
- **THEN** usage output includes `--fontName`, `--database`, `--width`, `--height`, `--dpi`, stylesheet, lat/lon, zoom, and output positional arguments

### Requirement: DrawMapSkia uses SkPngEncoder or PPM fallback
The demo SHALL encode the rendered bitmap as PNG using Skia's `SkPngEncoder` when available. If the system Skia installation lacks PNG encoding support, PPM output SHALL be used as a fallback (matching the AGG demo pattern).

#### Scenario: PNG written via SkPngEncoder
- **WHEN** `SkPngEncoder` is available and rendering completes successfully
- **THEN** the output is encoded using `SkPngEncoder::Encode` and written to the output file

#### Scenario: PPM fallback when SkPngEncoder unavailable
- **WHEN** `SkPngEncoder` is not available (e.g., broken Skia installation)
- **THEN** the demo writes PPM format using `WritePPM()`

### Requirement: DrawMapAll Skia backend registration
The `DrawMapAll` multi-backend demo SHALL conditionally include the Skia backend when `HAVE_OSMSCOUT_MAP_SKIA` is defined.

#### Scenario: Skia backend compiled in DrawMapAll
- **WHEN** building with Skia backend enabled
- **THEN** `DrawMapAll` includes `#include <osmscoutmapskia/MapPainterSkia.h>` and renders using `MapPainterSkia` when selected

#### Scenario: Skia backend skippable
- **WHEN** Skia backend is not enabled
- **THEN** `DrawMapAll` compiles without Skia references

### Requirement: Demo build integration (CMake)
The `Demos/CMakeLists.txt` SHALL conditionally build `DrawMapSkia` when `OSMSCOUT_BUILD_MAP_SKIA` is ON.

#### Scenario: DrawMapSkia target in CMake
- **WHEN** `OSMSCOUT_BUILD_MAP_SKIA` is ON
- **THEN** `DrawMapSkia` executable is created linking `OSMScout::OSMScout`, `OSMScout::Map`, and `OSMScout::MapSkia`

### Requirement: Demo build integration (Meson)
The `Demos/meson.build` SHALL conditionally build `DrawMapSkia` when `buildMapSkia` is true.

#### Scenario: DrawMapSkia target in Meson
- **WHEN** `buildMapSkia` is true
- **THEN** `DrawMapSkia` executable is created linking `osmscout`, `osmscoutmap`, and `osmscoutmapskia`

### Requirement: Demo config.h registration
The `Demos/src/meson.build` SHALL add `HAVE_LIB_OSMSCOUTMAPSKIA` to the `demoCfg` configuration data.

#### Scenario: Config key present
- **WHEN** inspecting `Demos/src/meson.build`
- **THEN** `demoCfg.set('HAVE_LIB_OSMSCOUTMAPSKIA', buildMapSkia, ...)` is present
