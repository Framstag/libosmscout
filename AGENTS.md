# AGENTS.md — libosmscout

## Project Identity

- **What**: C++20 library for offline OSM map rendering, routing, and location lookup
- **Version**: 1.1.1
- **Homepage**: http://libosmscout.sourceforge.net/
- **Support**: Matrix `#libosmscout.matrix.org`, mailing list on SourceForge

## Repository Map

Each directory is a standalone CMake/Meson subproject:

| Directory | Purpose |
|-----------|---------|
| `libosmscout/` | **Core library** — types, db, routing, location, IO, util |
| `libosmscout-map/` | Abstract map rendering layer |
| `libosmscout-map-agg/` | AGG (Anti-Grain Geometry) renderer |
| `libosmscout-map-cairo/` | Cairo renderer |
| `libosmscout-map-opengl/` | OpenGL renderer |
| `libosmscout-map-svg/` | SVG output renderer |
| `libosmscout-map-qt/` | Qt renderer |
| `libosmscout-map-directx/` | DirectX renderer (Windows) |
| `libosmscout-map-gdi/` | GDI renderer (Windows) |
| `libosmscout-map-iosx/` | iOS renderer |
| `libosmscout-map-binding/` | Map binding layer |
| `libosmscout-import/` | OSM → native binary format import pipeline |
| `libosmscout-client/` | Client query libraries |
| `libosmscout-client-qt/` | Qt client |
| `libosmscout-gpx/` | GPX data support |
| `libosmscout-binding/` | Java JNI bindings |
| `libosmscout-kotlin/` | Kotlin bindings |
| `libosmscout-extern/` | External dependency wrappers (glm, zlib, etc.) |
| `libosmscout-test/` | Test utilities, routing verification |
| `Demos/` | QML demo applications |
| `Tests/` | Test suite (Catch2-based) |
| `OSMScout2/` | Main desktop/mobile Qt app |
| `OSMScoutOpenGL/` | OpenGL-based app |
| `MCPServer/` | Map tile server (openapi2.json) |
| `Import/` | CLI import tool |
| `BasemapImport/` | Basemap-level import |
| `Android/` | Android apps (OsmScoutLib, OsmScoutViewer, OsmScoutBenchmark) |
| `Apple/` | macOS/iOS apps |
| `StyleEditor/` | Interactive style editor |
| `PublicTransportMap/` | Public transport demo |
| `WellScoutedRoute/` | Routing demo |
| `DumpData/` | Data debug/dump tool |
| `Java/` | Java examples (location lookup, routing, renderer, open db) |
| `stylesheets/` | `.oss` (styles) and `.ost` (type defs) style definitions |
| `Documentation/` | Build guides, style syntax docs, notes per platform |
| `setup/` | (empty — reserved for dev setup scripts) |
| `scripts/` | cppcheck.sh, etc. |
| `ci/` | Docker build configs |
| `packaging/` | Platform packaging |
| `webpage/` | Project website source |
| `subprojects/` | Meson subproject wraps (glm, glew, libxml2, zlib, liblzma, libpng, protobuf) |

## Build Systems

Two build systems maintained in parallel:

### CMake (primary)
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
- Root: `CMakeLists.txt`
- Each subproject has its own `CMakeLists.txt`
- Features gated by `OSMSCOUT_BUILD_*` options
- Qt version selectable via `QT_VERSION_PREFERRED` (5 or 6)
- macOS frameworks: `OSMSCOUT_BUILD_FRAMEWORKS`

### Meson (alternative)
```
meson setup build
meson compile -C build
```
- Root: `meson.build`
- Per-subproject `meson.build` files
- Supports shared/static via `default_library`

### Running Tests

CMake:
```bash
cd build && ctest -j 2 --output-on-failure
# With GUI tests (requires X server or xvfb):
cd build && xvfb-run ctest -j 2 --output-on-failure
# Single test:
cd build && ctest -R <TestName> --output-on-failure
```

Meson:
```bash
meson test --timeout-multiplier 2 -C build --print-errorlogs
```

Environment variables for tests:
- `QT_QPA_PLATFORM=offscreen` — required for running Qt-based tests without X server (e.g. in docker containers)
- `TESTS_TOP_DIR=<source>/Tests` — some tests need path to test data
- `TESTS_TMP_DIR=<build>/Tests` — some tests need writable temp dir

### Building with AddressSanitizer + UndefinedBehaviorSanitizer

```bash
cmake -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=undefined" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address -fsanitize=undefined" \
  -DOSMSCOUT_BUILD_TOOL_OSMSCOUT2=OFF \
  -DOSMSCOUT_BUILD_TOOL_OSMSCOUTOPENGL=OFF \
  -DOSMSCOUT_BUILD_DEMOS=OFF \
  -DOSMSCOUT_BUILD_TOOL_STYLEEDITOR=OFF \
  -DOSMSCOUT_BUILD_BINDING_JAVA=OFF \
  -DCMAKE_UNITY_BUILD=ON -G "Ninja"

cmake --build build-asan
cd build-asan && ctest -j 2 --output-on-failure --exclude-regex "PerformanceTest"
```

Notes:
- PerformanceTest is excluded because UI libraries leak on exit; run separately with `ASAN_OPTIONS=detect_leaks=0`
- Works with both GCC and Clang
- CI runs this on every PR (see `.github/workflows/sanitize_on_ubuntu_24_04.yml`)

### Dependencies
- **Conan**: `conanfile.txt` (loaded automatically when `conanbuildinfo.cmake` exists)
- **vcpkg**: Three profiles — `vcpkg_full.json`, `vcpkg_medium.json`, `vcpkg_minimum.json`
- **Subprojects** (Meson): wraps in `subprojects/`
- **System**: Standard OSM dependencies (libxml2, protobuf, libpng, zlib, etc.)

## CI/CD

GitHub Actions in `.github/workflows/`:

| Workflow | Platform |
|----------|----------|
| `build_and_test_on_ubuntu_24_04.yml` | Linux (gcc/clang) |
| `build_and_test_on_osx.yml` | macOS |
| `build_and_test_on_ios.yml` | iOS |
| `build_and_test_on_msys.yml` | Windows/MinGW |
| `build_and_test_on_vs2025.yml` | Windows/MSVC |
| `build_on_ubuntu_22_04_qt_android.yml` | Android NDK |
| `sanitize_on_ubuntu_24_04.yml` | Sanitizer builds |
| `sonar.yml` | SonarQube analysis |
| `release.yml` / `release_latest.yml` | Release automation |
| `webpage.yml` | Website build |

## Code Conventions

### Language & Standards
- **C++20** with `CMAKE_CXX_EXTENSIONS OFF` (no GNU extensions)
- MSVC: `/bigobj`, `/fp:fast`, selected warning disables (`/wd4251`, `/wd4456`, `/wd4458`)
- macOS: ObjC++ ARC enabled for platform code
- Namespace: `osmscout` for all public API
- No STL replacement — standard C++ throughout

### File Layout (core library pattern)
```
libosmscout/
  include/osmscout/      ← Public headers
    db/  routing/  location/  io/  util/  feature/  system/
    elevation/  navigation/  async/  cli/  log/  ost/  poi/
    projection/  private/
  src/osmscout/           ← Implementation files (mirrors include layout)
```

### Style & Quality
- Formatting: `.uncrustify` config (Uncrustify)
- Static analysis: `.clang-tidy` config
- Spell check: `cspell.json`
- Unit tests: `Tests/` (Catch2-based)
- No mocking framework observed
- Avoid asserts that user input can trigger

### Style Files
- `.oss` — Style definitions (e.g., `standard.oss`, `cycle.oss`, `railways.oss`)
- `.ost` — Type definitions (e.g., `map.ost`, `contour_lines.ost`)
- Located in `stylesheets/`

### Error Handling
- Moving toward Status-based error codes
- Legacy code may still use asserts

## C++ Coding Style

See [CODING_STYLE.md](CODING_STYLE.md) for full guide derived from actual code.
Covers naming, indentation, braces, classes, methods, pointers, enums,
include order, comments, formatting, header guards, templates, and error handling.
## Architecture Overview

```
                     +--------------------+
                     |  OSM XML / PBF     |
                     +--------+-----------+
                              |
                              v
                     +--------+-----------+
                     |  libosmscout-import |  ← OSM → native binary
                     +--------+-----------+
                              |
         +--------------------+--------------------+
         v                                         v
+--------+----------+                +-------------+---------+
|  libosmscout      |                | libosmscout-client    |
|  Core DB / types  |                | Query API             |
|  Routing          |                +-----------------------+
|  Location lookup  |
|  Geo math / IO    |
+--------+----------+
         |
         v
+--------+----------+
|  libosmscout-map   |  ← Abstract rendering API
+--------+----------+
         |
         +-----+------+------+------+-----+------+------+------+
         v      v      v      v      v     v      v      v      v
       AGG   Cairo  OpenGL   Qt    SVG  DirectX  GDI   iOSX  Binding
```

### Module Quick Reference (libosmscout core)

| Module | Description |
|--------|-------------|
| `db/` | Database I/O — type registry, tile storage, area/way/node indexes |
| `routing/` | Routing algorithm, data structures, cost functions |
| `location/` | Location lookup, reverse geocoding, address search |
| `io/` | Low-level file I/O — MD5, CRC, file readers/writers, compression |
| `util/` | Geometry (GeoCoord, Pixel, Point), projections, math helpers |
| `feature/` | OSM feature definitions mapped to rendering |
| `system/` | Platform abstractions (clock, thread, memory mapped files) |
| `elevation/` | Elevation / SRTM data handling |
| `navigation/` | Turn-by-turn navigation instruction generation |
| `async/` | Async execution, parallel processing, cancellation |
| `cli/` | Command-line flag parsing utilities |
| `log/` | Logging framework |
| `ost/` | OSM Style Template processing |
| `poi/` | Points of interest handling |
| `projection/` | Map projection implementations (Mercator, etc.) |
| `private/` | Internal implementation details, not public API |

### Key Data Types
- `Node`, `Way`, `Area` — Core OSM geometry types
- `TypeConfig`, `TypeInfoSet` — Type system for OSM tags → renderable features
- `Tag` — Key/value tag pairs
- `GeoCoord` — Geographical coordinate
- `Pixel` — Raster screen coordinate
- `Route`, `RouteDescription` — Routing results
- `GroundTile`, `PublicTransport` — Specialized data
- `ObjectRef` — Generic reference to any OSM object

## Conventions for AI Agents

### Navigation
- **Headers first**: Public API contracts live in `include/osmscout/`. Start here.
- **Implementation**: `src/osmscout/` mirrors header layout
- **Rendering changes**: Find the right map backend in `libosmscout-map-{name}/`
- **Import pipeline**: Code in `libosmscout-import/`
- **Style/visual changes**: `stylesheets/` — `.oss` (rules), `.ost` (type mapping)
- **Tests**: `Tests/` for unit tests, `libosmscout-test/` for test utilities

### Common Patterns
- No DI framework — objects are constructed manually
- Most classes accept `osmscout::TypeConfigRef` for type information
- File I/O uses custom scanner/writer classes in `osmscout::io`
- Map rendering goes through `MapRenderer` interface per backend
- Styles are applied via `StyleConfig` which loads `.oss`/`.ost` files

### Pitfalls
- Two build systems (CMake + Meson) both must be updated for structural changes
- Assert violations on invalid user data are considered bugs
- Platform-specific code scattered across `#ifdef` and `system/` — check Android, Apple, Win32 guards
