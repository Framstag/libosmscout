## Context

CMake build system in libosmscout hardcodes `lib` as the install destination for libraries, pkg-config files, and config cmake files. The `GNUInstallDirs` module exists precisely to solve this — it detects the platform convention (`lib64` on Fedora/RHEL, `lib/x86_64-linux-gnu` on Debian multiarch, `lib` elsewhere).

The Meson build system already does this correctly via its built-in `libdir` handling.

The affected code is centralized:
- `cmake/ProjectConfig.cmake` — macros `osmscout_library_project`, `osmscout_demo_project`, `osmscout_test_project`
- Root `CMakeLists.txt` — config cmake install dir + stylesheets install
- A few tool `CMakeLists.txt` files

## Goals / Non-Goals

**Goals:**
- `.so` files install to correct platform libdir
- `.pc` (pkg-config) files install to correct platform libdir
- CMake config files install to correct platform libdir
- Consistent `BINDIR` usage for executables
- Zero behavioral change for non-install builds

**Non-Goals:**
- Changing Meson build system
- Changing the import pipeline or data formats
- Fixing Windows-specific install paths (separate concern)

## Decisions

1. **Use `GNUInstallDirs` over manual `CMAKE_INSTALL_LIBDIR`**
   - `GNUInstallDirs` is the standard CMake module, handles multiarch, respects `CMAKE_INSTALL_LIBDIR` cache variable, works on all platforms
   - Alternative: manual `if(CMAKE_SIZEOF_VOID_P EQUAL 8)`. Rejected — doesn't handle Debian multiarch, distro packaging overrides

2. **Include `GNUInstallDirs` once in root `CMakeLists.txt`**
   - All subproject CMakeLists.txt inherit it via `add_subdirectory` scope
   - Must be placed after `project()` (needs languages enabled) but before any `install()` call
   - The macros in `ProjectConfig.cmake` are invoked at subdirectory scope, so they'll see the variable

3. **Replace hardcoded `lib` → `${CMAKE_INSTALL_LIBDIR}`**
   - All 5+ locations in `ProjectConfig.cmake` macros
   - Config install dir in root `CMakeLists.txt`
   - `MCPServer/CMakeLists.txt` ARCHIVE destination (executable, archive never produced, but for correctness)
   - Replace `DESTINATION bin` → `${CMAKE_INSTALL_BINDIR}` in the same macros for consistency

4. **Preserve relative paths**
   - `GNUInstallDirs` defaults are relative (`lib`, `bin`, `include`), so existing behavior on standard systems is unchanged
   - On Fedora `lib64`, the variable resolves to `lib64` and everything works correctly

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Multiarch paths (`lib/x86_64-linux-gnu`) require `DESTDIR` install for verification | Use `DESTDIR` test with `find` as validation strategy |
| Variables might be empty if `GNUInstallDirs` isn't included early enough | Include after `project()` but before any `install()` call |
| Existing CI installs might hardcode `lib` paths | All CI uses `cmake --install`, so paths will track correctly |
| Framework builds on macOS | `GNUInstallDirs` respects platform — `lib` on macOS is correct |
