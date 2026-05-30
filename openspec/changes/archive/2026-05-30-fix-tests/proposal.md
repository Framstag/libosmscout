## Why

Two tests fail on MinGW/MSYS2 (Windows CI): `HighwayMilestoneFeatureTest` crashes on hardcoded `/tmp/` paths that don't exist on Windows; `SymbolRendererSVGTest` exits with `STATUS_DLL_NOT_FOUND` (0xc0000135) due to missing runtime DLLs at test execution on Windows.

These failures block Windows CI green status and reduce coverage confidence — other developers can't trust the Windows build gate.

## What Changes

- **HighwayMilestoneFeatureTest**: Replace hardcoded `/tmp/highway_milestone_*.dat` paths with OS-agnostic temporary directory lookup (e.g., `std::filesystem::temp_directory_path()`). Keep files local to test scope — create, use, clean up. No behavior or logic changes to the feature itself.
- **SymbolRendererSVGTest**: Ensure the test executable can find required DLLs on Windows. The test links `osmscout`, `osmscoutmap`, and `osmscoutmapsvg` — the SVG renderer module may have additional dependency chain (e.g., libpng, zlib, xml2) not captured in test environment PATH. Fix may involve adjusting CMake test environment setup (`osmscout_test_project` macro's Win32 PATH logic) to include all transitive DLL directories, or verifying meson test env equivalently. No source code changes to the test logic itself expected.
- **No spec-level behavior changes**: The tests test existing behavior that is correct. Only test infrastructure and environment setup changes.

## Capabilities

### New Capabilities
*(none)*

### Modified Capabilities
*(none)*

## Impact

- `Tests/src/HighwayMilestoneFeatureTest.cpp` — 4 lines using `/tmp/` paths replaced
- `cmake/ProjectConfig.cmake` — `osmscout_test_project` macro WIN32 PATH logic may need extension for `osmscoutmapsvg` transitive DLLs
- `Tests/meson.build` — meson test environment may need env PATH adjustments for `osmscoutmapsvg` DLLs on Windows
- No API, ABI, or feature behavior changes
