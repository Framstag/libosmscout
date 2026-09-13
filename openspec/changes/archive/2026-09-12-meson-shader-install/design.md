## Context

See proposal.md — Why for motivation. Current state:

- CMake (`libosmscout-map-opengl/CMakeLists.txt`) defines `SHADER_INSTALL_DIR` per platform (Linux `${CMAKE_INSTALL_PREFIX}/share/osmscout/shaders`, Apple `/Library/Application Support/osmscout/shaders`, Windows `C:/ProgramData/osmscout/shaders`), installs `SHADER_FILES` and `FONT_FILES` there, sets `DEFAULT_FONT_FILE` to the fonts dir, and templates them into `MapOpenGLFeatures.h` via `MapOpenGLFeatures.h.cmake` (`#cmakedefine`).
- Meson (`libosmscout-map-opengl/include/osmscoutmapopengl/meson.build`) generates the same header via `configure_file` but sets `SHADER_INSTALL_DIR = "shaders"` (relative) and `DEFAULT_FONT_FILE = "LiberationSans-Regular.ttf"` (bare file name), with an inline `# TODO: setup installation dir properly`. The meson build installs only the library, headers, and font/license data — no shader install rules exist.
- Consumers defaulting to `SHADER_INSTALL_DIR`: `OSMScoutOpenGL/src/OSMScoutOpenGL.cpp` (Arguments default), `Demos/src/DrawMapOpenGL.cpp`, `Demos/src/DrawMapAll.cpp`. All offer a `--shaders` override. PerformanceTest passes `--shaders` explicitly from both build systems (fixed by `fix-opengl-performance-tests`), so tests are unaffected; only built-and-installed applications/demos hit the broken default.
- Meson builds and installs `OSMScoutOpenGL` (`install: true`); demos (DrawMapAll, DrawMapOpenGL) are also meson executables. The relative `"shaders"` default resolves only when the working directory of the running application contains a `shaders/` subdirectory.

## Goals / Non-Goals

**Goals:**
- `meson install` delivers shader (and font) files to the same platform directories CMake uses.
- Meson-generated `SHADER_INSTALL_DIR`/`DEFAULT_FONT_FILE` are absolute paths that match those directories.
- Meson-built applications/demos initialize the painter from the default path after `meson install`, from any working directory.
- CMake behavior unchanged.

**Non-Goals:**
- Making uninstalled (build-dir) runs resolve shaders by default — CMake doesn't provide that either; dev flow uses `--shaders` (already wired for tests and demos).
- Changing `MapPainterOpenGL` shader loading or the `--shaders` CLI semantics.
- Reworking the shared template between the two build systems beyond what path generation requires.

## Decisions

### D1: Mirror the CMake platform paths in meson and add install rules

Add `install_data()` rules for the shader and font files in `libosmscout-map-opengl/meson.build`, and set `SHADER_INSTALL_DIR`/`DEFAULT_FONT_FILE` in the meson `configure_file` to the same absolute platform paths CMake uses (via the same `host_machine.system()` conditional: Linux `share/osmscout/shaders`, Apple Application Support, Windows ProgramData).

- **Alternative A (chosen)**: Absolute install paths + `install_data()`, exactly mirroring CMake. One source of truth for paths (copy the CMake mapping), installed-usage works identically across build systems, no new runtime lookup logic.
- **Alternative B**: Copy shaders into the meson build dir with a `custom_target` and point `SHADER_INSTALL_DIR` at the build dir. Makes uninstalled runs work, but diverges from CMake, adds a copy per shader, and install would still be missing for installed usage — rejected.
- **Alternative C**: Keep the relative `"shaders"` default and teach consumers to search relative to the binary's location. Runtime path-guessing in every consumer, platform-dependent (bundles, Windows), and breaks the installed CMake parity — rejected.

Risk assessment: A is low risk — it copies working CMake behavior into meson; the main risks are (1) path divergence between the two build files over time, mitigated by keeping the mapping structurally comment-linked to CMakeLists.txt, and (2) prefix handling: meson needs the install prefix at configure time (`get_option('prefix')`) to write the absolute path into the header, which is the same configure-time assumption CMake makes via `CMAKE_INSTALL_PREFIX`.

### D2: Keep the `--shaders` override as the dev-flow escape hatch

No changes to the CLI options in consumers (OSMScoutOpenGL, DrawMapOpenGL, DrawMapAll) or the explicit `--shaders` arguments in test registrations. After D1, `--shaders` remains the documented way to run meson-built executables without installing.

- **Alternative A (chosen)**: Keep override + install-provided default. Matches CMake flow exactly; zero consumer code changes.
- **Alternative B**: Also search build-dir copies automatically. Extra lookup logic with platform-specific edge cases, unnecessary because `--shaders` already covers dev runs.

Risk assessment: A is lowest risk; B is rejected as out of scope (see Non-Goals).

## Risks / Trade-offs

- [Prefix at configure time] → Same assumption as CMake; a meson `--prefix` change after configure requires reconfiguration, identical to CMake behavior.
- [Path duplication between CMakeLists.txt and meson.build] → Keep a comment in each pointing at the other; the acceptance test (installed file set + header value) catches drift.
- [Windows/Apple paths in meson CI] → Meson CI (`javascout-ci-linux-meson` workflows) is Linux; Apple/Windows paths are mirrored by code inspection, Linux path verified by test.

## Migration Plan

No data migration. Existing meson install prefix re-installation restores shader placement; a stale configured `MapOpenGLFeatures.h` regenerates on reconfigure. Rollback: revert meson build-file changes; CMake unaffected either way.

## Open Questions

- Should the meson `DEFAULT_FONT_FILE` fall back to a source-tree font for uninstalled runs (like the shader `--shaders` flow)? Deferrable — not raised by the failing scenario; revisit if font resolution becomes a meson pain point.
