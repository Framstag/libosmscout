## Why

CMake install destinations for libraries are hardcoded to `lib/`, ignoring platform conventions. On 64-bit Fedora/RHEL this means `.so` files land in `lib/` instead of `lib64/`. On Debian multiarch it means they miss the `lib/x86_64-linux-gnu/` path. Meson already handles this correctly. The CMake build should match.

## What Changes

- Add `include(GNUInstallDirs)` to root `CMakeLists.txt`
- Replace all hardcoded `DESTINATION lib` with `${CMAKE_INSTALL_LIBDIR}` in:
  - `cmake/ProjectConfig.cmake` (the `osmscout_library_project`, `osmscout_demo_project`, and `osmscout_test_project` macros)
  - root `CMakeLists.txt` (config cmake install path)
  - `MCPServer/CMakeLists.txt` (ARCHIVE destination)
- Replace hardcoded `DESTINATION bin` with `${CMAKE_INSTALL_BINDIR}` for consistency (no reported bug, but follows same principle)
- A few tool `CMakeLists.txt` files hardcode `lib` for ARCHIVE (executables don't produce archives) - clean those too

## Capabilities

### New Capabilities
- `cmake-install-libdir`: Correct library installation directory detection using `GNUInstallDirs`

### Modified Capabilities
*(none — no spec-level behavior changes, only build system installation paths)*

## Impact

- Build system only (CMake). Meson unchanged.
- No API/ABI changes.
- No behavior changes at runtime.
- Changes install output paths for all subproject libraries, demos, tests, and .pc files.
