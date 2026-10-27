# Verification

Delta: two removed `REQUIRED` keywords and a three-line comment in `cmake/FindPango.cmake`. No compiled
file is in the delta, so the test suite is not the gate; the gate is the configure matrix below, which is
also the file's own check (`cmake/FindPango.cmake` is only ever `include()`d, and every run of the matrix
parses it).

Host: Linux, CMake configure times are the `-- Configuring done (8.9s)` line of each log. All times below
are UTC and were taken when the command ran: the retained logs carry no clock stamp of their own
(`grep -c '2026-10'` returns 0 for every file under `evidence/`), so the order of the runs rests on the
file mtimes alone.

The `/tmp/101-*` paths in the commands below are the scratch build directories the runs used, not
evidence: every log and every number they carry is retained beside this file as
`red-*.log` / `green-*.log`, and the configured headers as `green-features-*.h`.

## Red - before the fix (`evidence/red-configure-*.log`)

| run | command | exit | start / end |
|---|---|---|---|
| r1 | `cmake -B /tmp/101-red-freetype -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Freetype=ON` | 1 | 2026-10-16T10:50:19Z / 10:50:27Z |
| r2 | `cmake -B /tmp/101-red-fontconfig -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Fontconfig=ON` | 1 | 2026-10-16T10:50:34Z / 10:50:43Z |

r1 disables only FreeType: it reports `cmake/FindPango.cmake:112`, goes on to `-- Found Fontconfig`, and
fails a second time at `CMakeLists.txt:341` (the OpenGL map backend's own FreeType requirement, which
`-DOSMSCOUT_BUILD_MAP_OPENGL=OFF` switches off and r1 does not set); r2 disables only Fontconfig and
reports `cmake/FindPango.cmake:116`. Neither log names the other's call, so neither run covers both.

```
CMake Error at cmake/FindPango.cmake:112 (FIND_PACKAGE):
  FIND_PACKAGE for module Freetype called with REQUIRED, but
  CMAKE_DISABLE_FIND_PACKAGE_Freetype is enabled.  A REQUIRED package cannot
  be disabled.
Call Stack (most recent call first):
  cmake/features.cmake:257 (find_package)
  CMakeLists.txt:72 (include)
```

```
CMake Error at cmake/FindPango.cmake:116 (FIND_PACKAGE):
  FIND_PACKAGE for module Fontconfig called with REQUIRED, but
  CMAKE_DISABLE_FIND_PACKAGE_Fontconfig is enabled.  A REQUIRED package cannot
  be disabled.
Call Stack (most recent call first):
  cmake/features.cmake:257 (find_package)
  CMakeLists.txt:72 (include)
```

## Green - after the fix

| run | command | exit | start / end | evidence |
|---|---|---|---|---|
| a | `cmake -B /tmp/101-a-nofreetype -DCMAKE_BUILD_TYPE=Release -DOSMSCOUT_BUILD_MAP_OPENGL=OFF -DCMAKE_DISABLE_FIND_PACKAGE_Freetype=ON` | 0 | 2026-10-16T10:51:17Z / 10:51:26Z | `green-configure-disable-freetype.log`, `green-features-disable-freetype.h` |
| b | `cmake -B /tmp/101-b-nofontconfig -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Fontconfig=ON` | 0 | 2026-10-16T10:51:28Z / 10:51:37Z | `green-configure-disable-fontconfig.log`, `green-features-disable-fontconfig.h` |
| c | `cmake -B /tmp/101-c-normal -DCMAKE_BUILD_TYPE=Release` | 0 | 2026-10-16T10:51:40Z / 10:51:48Z | `green-configure-default.log`, `green-features-default.h` |
| d1 | `cmake --build /tmp/101-a-nofreetype --target OSMScoutMapCairo -j 16` | 0 | 2026-10-16T10:51:50Z / 10:53:13Z | `green-build-disable-freetype.log` |
| d2 | `cmake --build /tmp/101-b-nofontconfig --target OSMScoutMapCairo -j 16` | 0 | 2026-10-16T10:53:16Z / 10:54:43Z | `green-build-disable-fontconfig.log` |

Feature gates in the configured `libosmscout-map-cairo/include/osmscoutmapcairo/MapCairoFeatures.h`:

| run | `..._HAVE_LIB_FREETYPE` | `..._HAVE_LIB_FONTCONFIG` | `..._HAVE_LIB_CAIRO_FT` |
|---|---|---|---|
| a | undefined | defined | undefined |
| b | defined | undefined | defined |
| c | defined | defined | defined |

Run c also reports the two packages it is expected to keep: `Found Freetype: /usr/lib/libfreetype.so
(found version "2.14.3")` and `Found Fontconfig: /usr/lib/libfontconfig.so (found version "2.18.3")`
(`green-configure-default.log:96`, `:138`).

Both builds of d end on `[100%] Built target OSMScoutMapCairo` with zero `error:`/`warning:` lines
(`grep -ci 'warning:\|error:'` returned 0 for both logs), so the Cairo backend compiles with either
feature gate off.

`-DOSMSCOUT_BUILD_MAP_OPENGL=OFF` in run a: `CMAKE_DISABLE_FIND_PACKAGE_Freetype=ON` makes
`Freetype::Freetype` unavailable, which the OpenGL map backend requires for its text loader
(`CMakeLists.txt:341`, `libosmscout-map-opengl/src/osmscoutmapopengl/TextLoader.cpp:31`). That backend's
requirement is unaffected by this change; run a disables the backend so the run isolates the find
module. Without the flag that configure still fails, as it should.

## Not verified here

- Run on a host without the packages installed: out of scope, the configure matrix covers the disabled
  case the defect is about, and the include-directory guards it leaves in place are the pre-existing
  code path for a missing package.
- The Meson build needs no equivalent change (`libosmscout-map-cairo/meson.build:31`, `:35`;
  `meson.build:182`, `:214` already resolve both optionally) and was therefore not reconfigured.
