# Design

## Context

See `proposal.md` — Why. The relevant current state is in `libosmscout-map-opengl/CMakeLists.txt`:

- The Linux branch defines `SHADER_INSTALL_DIR` and `FONTS_INSTALL_DIR` as `${CMAKE_INSTALL_PREFIX}/share/osmscout/{shaders,fonts}` and passes them to `install(FILES ... DESTINATION ...)`. CMake treats an absolute destination as final, so `--prefix` at install time cannot override it; the Apple and Windows branches use fixed platform locations for the same reason.
- `DEFAULT_FONT_FILE` is derived from `FONTS_INSTALL_DIR`, and `MapOpenGLFeatures.h.cmake` turns `SHADER_INSTALL_DIR` and `DEFAULT_FONT_FILE` into compile-time defines used by `OSMScoutOpenGL/src/OSMScoutOpenGL.cpp:45`, `Demos/src/DrawMapOpenGL.cpp:102`, `Demos/src/DrawMapAll.cpp:196` and `Tests/src/PerformanceTest.cpp:125`.
- `GNUInstallDirs` is already included once at the root (`CMakeLists.txt:4`), as required by the `cmake-install-libdir` capability, so `CMAKE_INSTALL_DATADIR` and `CMAKE_INSTALL_FULL_DATADIR` are available in this subdirectory.
- `openspec/specs/meson-shader-install` requires the Meson-generated header to report absolute `SHADER_INSTALL_DIR`/`DEFAULT_FONT_FILE` values at parity with CMake, so the CMake-generated values must stay absolute and unchanged for a default configure.

## Goals / Non-Goals

**Goals:**

- The `install(...)` destination for shaders and fonts follows an install prefix chosen at install time.
- The generated `MapOpenGLFeatures.h` values are unchanged for a default configure, and a normal install keeps delivering resources to the configure-time prefix.
- One subdirectory change; no consumer code, no template change.

**Non-Goals:**

- Changing the Apple/Windows platform locations, the Meson install rules, or the `SHADER_INSTALL_DIR`/`DEFAULT_FONT_FILE` contract.
- The MSVC PDB install destination in `cmake/ProjectConfig.cmake:138`, which has the same shape but is Windows-only and not verifiable in this environment.
- Making the compiled-in default follow an install-time prefix override; the header is generated at configure time and cannot know it.

## Decisions

### Decision 1: Separate the install destination from the compiled-in default

Introduce prefix-relative destination variables for the `install()` calls (from `${CMAKE_INSTALL_DATADIR}` on the Linux branch) and keep the absolute compiled-in values that feed the generated header.

- Alternatives considered:
  - **Keep one absolute `SHADER_INSTALL_DIR` and accept the `--prefix` limitation.** Rejected: that is the defect; it also aborts a full-tree install, so it blocks verifying install rules of later subdirectories.
  - **Make `SHADER_INSTALL_DIR` relative and let consumers resolve it.** Rejected: it changes the define contract and would break `OSMScoutOpenGL`, the demos and `PerformanceTest`, and it contradicts the `meson-shader-install` parity requirement.
  - **Install-time path substitution via a generator expression in `DESTINATION`.** Not available: `install(DESTINATION)` does not expand the install prefix that way, and using `$<INSTALL_PREFIX>` is not a valid destination expression.

### Decision 2: Derive the compiled-in absolute path from `CMAKE_INSTALL_FULL_DATADIR`

The header values keep their current default spelling on a default configure, but are expressed through GNUInstallDirs (`CMAKE_INSTALL_FULL_DATADIR`) instead of a hand-written `share` component, so a custom `CMAKE_INSTALL_DATADIR` is reflected in both the destination and the compiled-in default.

- Alternatives considered:
  - **Leave the default as `${CMAKE_INSTALL_PREFIX}/share/osmscout/...`.** Rejected: a non-default data directory would install somewhere the compiled-in default does not name.
  - **Hard-code `share` in both.** Rejected: ignores a supported GNUInstallDirs override.

The install destination is `${CMAKE_INSTALL_DATADIR}` when that variable is set and `${CMAKE_INSTALL_DATAROOTDIR}` otherwise. `GNUInstallDirs` may leave `CMAKE_INSTALL_DATADIR` empty (it then means `CMAKE_INSTALL_DATAROOTDIR`), and an empty component would turn `${_osmscoutInstallDatadir}/osmscout/shaders` into the absolute path `/osmscout/shaders`, reproducing the defect instead of fixing it. `CMAKE_INSTALL_FULL_DATADIR` already resolves the empty case, so the compiled-in default needs no such fallback.

### Decision 3: Leave Apple/Windows and Meson untouched

The Apple and Windows branches keep their platform locations (outside any prefix), matching the Meson side.

- Alternatives considered:
  - **Make all platforms prefix-relative.** Rejected: changes an established platform convention, requires packaging changes outside this repository, and breaks CMake/Meson parity that `meson-shader-install` records.

No non-trivial runtime flow is introduced; the change is confined to configure-time variables and two generated install rules, so no sequence diagram applies.

## Risks / Trade-offs

- [An install-time `--prefix` that differs from the configure-time prefix leaves the compiled-in default pointing at the configure-time prefix] → Pre-existing and inherent to a configure-time generated header; the Meson side already behaves this way, and `--shaders`/`--font` remain the documented override. The new scenarios assert the default is correct for a normal install, not for a mismatched override.
- [`CMAKE_INSTALL_DATADIR` can be absolute if a caller passes an absolute value] → CMake then keeps treating the destination as absolute and `--prefix` does not apply; that is the caller's explicit request and is acceptable.
- [The generated header values drift from the Meson-generated ones] → Keep the default-configure values byte-identical; the `meson-shader-install` scenarios already check the Meson side, and the new scenario checks the CMake side.
- [The fix is mistaken for closing the whole `CMAKE_INSTALL_PREFIX`-in-destination class] → The MSVC PDB occurrence is named in `proposal.md` and stays open.

## Migration Plan

None required. The change touches build configuration only; a rebuild and reinstall pick it up, and reverting the commit restores the previous destinations. Rollback: revert the change.

## Open Questions

None.
