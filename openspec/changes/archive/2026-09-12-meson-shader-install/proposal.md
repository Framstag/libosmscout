## Why

Meson builds generate `MapOpenGLFeatures.h` with `SHADER_INSTALL_DIR` set to the relative path `"shaders"` (`libosmscout-map-opengl/include/osmscoutmapopengl/meson.build`). The OpenGL consumers that default to this path — `OSMScoutOpenGL`, `DrawMapOpenGL`, `DrawMapAll` — only find shaders when the working directory happens to contain a `shaders/` subdirectory. CMake has no such problem: it installs the shader files and generates an absolute platform-specific install path (`${CMAKE_INSTALL_PREFIX}/share/osmscout/shaders` on Linux). Meson additionally installs no shader files at all. Result: after a `meson install`, the meson-built OpenGL applications cannot initialize the painter from any normal working directory. The `fix-opengl-performance-tests` change solved this for PerformanceTest by passing `--shaders` explicitly; the applications and demos still rely on the broken default.

## What Changes

- Meson builds deliver the OpenGL shader files where consumers can find them, at parity with CMake: installed to a platform-appropriate directory by `meson install`, and the generated `MapOpenGLFeatures.h` pointing at an absolute path.
- Meson-built `OSMScoutOpenGL`, `DrawMapOpenGL`, and `DrawMapAll` initialize the OpenGL painter from their default shader path after `meson install`, without a `--shaders` argument.
- The existing explicit `--shaders` override keeps working (used by PerformanceTest and the demos).

## Capabilities

### New Capabilities
- `meson-shader-install`: Delivery and default resolution of OpenGL shader resources in meson builds — where shader files are installed, what path the meson-generated feature header reports, and how applications resolve them by default.

### Modified Capabilities
<!-- No existing spec-level behavior changes. -->

## Impact

- `libosmscout-map-opengl/meson.build` — install rules for the shader files (and, for parity, the font files) previously only installed by CMake.
- `libosmscout-map-opengl/include/osmscoutmapopengl/meson.build` — `SHADER_INSTALL_DIR` (and `DEFAULT_FONT_FILE`) value generation: relative `"shaders"` → absolute install path, mirroring the CMake template `MapOpenGLFeatures.h.cmake`.
- `libosmscout-map-opengl/include/osmscoutmapopengl/MapOpenGLFeatures.h.cmake` — unchanged, unless the platform path set is factored into a shared place; CMake behavior must not change.
- `OSMScoutOpenGL/meson.build` — no code change expected if the generated header defaults are fixed; verify only.
- `Demos/` (DrawMapOpenGL, DrawMapAll) — no code change expected; verify default shader resolution after `meson install`.
- `Tests/meson.build` — already passes `--shaders` explicitly; verify it still passes unchanged.
