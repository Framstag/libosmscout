# Proposal

## Why

`cmake --install build --prefix <dir>` aborts in the OpenGL backend instead of installing into `<dir>`: the shader and font install rules bake the configure-time `CMAKE_INSTALL_PREFIX` into the install destination, so the generated install script writes to the absolute configure-time path and ignores the overriding prefix (`file cannot create directory: /usr/local/share/osmscout/shaders. Maybe need administrative privileges.`). A full-tree install therefore never reaches the later subdirectories, which also blocks verifying the install rules of anything installed after the OpenGL backend.

## What Changes

- The OpenGL shader and font install rules on Linux become prefix-relative: an install that overrides the prefix places the files under the overriding prefix, and an install into the configure-time default keeps the current layout (`<prefix>/share/osmscout/shaders`, `<prefix>/share/osmscout/fonts`).
- The compiled-in default paths reported by the generated `MapOpenGLFeatures.h` (`SHADER_INSTALL_DIR`, `DEFAULT_FONT_FILE`) stay absolute and keep pointing at the platform shader/font directory, so `OSMScoutOpenGL`, `DrawMapOpenGL` and `DrawMapAll` still initialize the painter from their default shader path after a normal install, and the `--shaders` override keeps precedence.
- The Apple and Windows install locations (`/Library/Application Support/osmscout/...`, `C:/ProgramData/osmscout/...`) are platform conventions outside the install prefix and stay unchanged, as does the Meson side, which already installs under its prefix.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `cmake-install-libdir`: the capability currently covers `libdir`, `bindir` and pkg-config/config destinations; it gains the requirement that the OpenGL shader and font files install under the `GNUInstallDirs` data directory so an overriding install prefix is honored, without changing the absolute compiled-in default paths.

## Impact

- `libosmscout-map-opengl/CMakeLists.txt` — the `SHADER_INSTALL_DIR`/`FONTS_INSTALL_DIR` definitions and the two `install(FILES ...)` rules; the `DEFAULT_FONT_FILE` definition that consumers compile in.
- `libosmscout-map-opengl/include/osmscoutmapopengl/MapOpenGLFeatures.h.cmake` — unchanged; the absolute `SHADER_INSTALL_DIR`/`DEFAULT_FONT_FILE` contract it documents stays.
- Consumers that reference the compiled-in defaults (`OSMScoutOpenGL/src/OSMScoutOpenGL.cpp`, `Demos/src/DrawMapOpenGL.cpp`, `Demos/src/DrawMapAll.cpp`, `Tests/src/PerformanceTest.cpp`) — verify only, no code change expected.
- `.github/workflows/build_and test_on_ubuntu_24_04.yml`, `sanitize_on_ubuntu_24_04.yml`, `build_javascout.yml` — the existing `sudo cmake --install build` steps keep working; the override path is the new verification.
- Related, deliberately out of scope: `cmake/ProjectConfig.cmake:138` installs the MSVC PDB file with the same absolute-prefix pattern; it is MSVC-only and needs a Windows run to verify, so it is recorded rather than fixed here.
