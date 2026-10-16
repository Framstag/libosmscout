## Why

`cmake/FindPango.cmake` makes FreeType and fontconfig hard requirements whenever pangoft2 is present,
so a configure that disables either of the two font dependencies the backends feature-gate fails before
it can be used to build or verify anything.

Root cause: the pangoft2 branch of `cmake/FindPango.cmake` called `FIND_PACKAGE(Freetype REQUIRED)` and
`FIND_PACKAGE(Fontconfig REQUIRED)` inside its `IF(PANGOFT2_INCLUDE_DIRS)` block (lines 112 and 116 of
the file before the fix, 115 and 119 after it), so CMake rejects a configure
(`CMAKE_DISABLE_FIND_PACKAGE_<name>`) that disables either optional, feature-gated font dependency.

Evidence (red, on the revision before the fix):

```
$ cmake -B /tmp/101-red-freetype -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Freetype=ON
CMake Error at cmake/FindPango.cmake:112 (FIND_PACKAGE):
  FIND_PACKAGE for module Freetype called with REQUIRED, but
  CMAKE_DISABLE_FIND_PACKAGE_Freetype is enabled.  A REQUIRED package cannot
  be disabled.
Call Stack (most recent call first):
  cmake/features.cmake:257 (find_package)
  CMakeLists.txt:72 (include)
```

```
$ cmake -B /tmp/101-red-fontconfig -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Fontconfig=ON
CMake Error at cmake/FindPango.cmake:116 (FIND_PACKAGE):
  FIND_PACKAGE for module Fontconfig called with REQUIRED, but
  CMAKE_DISABLE_FIND_PACKAGE_Fontconfig is enabled.  A REQUIRED package cannot
  be disabled.
Call Stack (most recent call first):
  cmake/features.cmake:257 (find_package)
  CMakeLists.txt:72 (include)
```

Both runs exited 1 and their logs are retained as `evidence/red-configure-disable-freetype.log` and
`evidence/red-configure-disable-fontconfig.log`. Neither retained log carries a clock stamp of its own,
so the run times quoted in `evidence/verification.md` are the ones taken when each command ran.

Two runs are required because each disables a different package and neither log names the other's call:
r1 disables only FreeType - it reports `cmake/FindPango.cmake:112`, goes on to `-- Found Fontconfig`, and
fails a second time at `CMakeLists.txt:341` (the OpenGL map backend requires FreeType itself, and r1 does
not set `-DOSMSCOUT_BUILD_MAP_OPENGL=OFF`) - so its log never names the `:116` call; r2 disables only
Fontconfig and reports `cmake/FindPango.cmake:116`.

The dependencies are optional everywhere else. `cmake/features.cmake:227` calls `find_package(Freetype)`
and only records `HAVE_LIB_FREETYPE` through `target_exists(Freetype::Freetype ...)`, `:264` calls
`find_package(Fontconfig QUIET)` with the comment that the font configuration "is not a build
requirement", `:271-272` derive `OSMSCOUT_MAP_CAIRO_HAVE_LIB_FREETYPE` and
`OSMSCOUT_MAP_CAIRO_HAVE_LIB_FONTCONFIG` from those two results, and the Cairo painter consumes them as
`#if defined(...)` feature gates (`libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp:44`,
`:85`) with the `#cmakedefine`s at
`libosmscout-map-cairo/include/osmscoutmapcairo/MapCairoFeatures.h.cmake:11` and `:16`. Neither
`REQUIRED` call feeds any of them: they only append include directories inside the `IF(..._FOUND)`
guards that follow them. `libosmscout-map-svg/CMakeLists.txt:26` finds fontconfig with `QUIET` as well.
`AGENTS.md` ("Dependencies") names both as optional and feature-gated.

Why now: verifying the no-op branch of the font name resolution in `fix-cairo-font-file-resolution`
wanted exactly this configure - a build without FreeType - and had to compile a unit on its own instead
(that change's `verification.md`); `TODO.md` §101 records what blocked it.

## What Changes

- A dependency the backends feature-gate SHALL NOT be a configure requirement: a configure of the
  project with FreeType or the font configuration disabled SHALL complete, and with either disabled the
  corresponding feature gate of the Cairo backend SHALL be off in the configured
  `libosmscout-map-cairo/include/osmscoutmapcairo/MapCairoFeatures.h`.
- A configure that leaves both enabled SHALL still find both and define both feature gates, so the
  supported font paths of the Cairo backend stay compiled in.
- Explicitly not changed: the rest of `cmake/FindPango.cmake` (its find mechanism, its pkg-config calls,
  its `HANDLE_STANDARD_ARGS` results), the list of variables it sets, `cmake/features.cmake` and its
  feature gates, the Meson build (already optional, `libosmscout-map-cairo/meson.build:31`, `:35`), and
  the requirement of the OpenGL backend for FreeType (`CMakeLists.txt:341`), which is a genuine
  requirement of that backend and is reported as the configuration error it is.

## Impact

- Affected files: `cmake/FindPango.cmake` only. No compiled source, no configured template, no public
  header, no build-system structure.
- Affected modules: the configure step that locates Pango for `libosmscout-map-cairo` and
  `libosmscout-map-svg`; nothing at build or run time.
- Verification: the configure matrix in `tasks.md` - a configure with FreeType disabled, one with
  fontconfig disabled, one with both present, and the compilation of the `OSMScoutMapCairo` target in
  the two disabled configurations.
- No CI workflow relies on the hard requirement; the check is recorded in `design.md`.
