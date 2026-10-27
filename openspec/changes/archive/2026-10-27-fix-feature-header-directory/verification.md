# Verification

Date: 2026-10-27. Host CMake: 4.4.4 (gcc 16.2.1).

## What this change can and cannot be shown to do here

The defect it removes is a `cmake_path(GET … PARENT_PATH …)` result that differs between CMake versions; the
CMake on this host is 4.4.4, which applies the in-place form correctly. Reproducing the wrong path would need
the CMake 3.22 the Android Gradle plugin ships, which is not available on this machine (and its SDK lives
outside this repository). The checks below therefore pin the observable result — every module's rendered
header at `<build>/<module>/include/<includedir>/<Name>Features.h`, no `<Name>Features.h.cmake/` path, the
header installed, the core library compiling against it — and the removal of the fragile form from the macro.

Cross-check for the claim itself: CMake's `cmake_path` documentation describes `GET` subcommands as taking an
input variable and a mandatory output variable (the in-place form is not part of that contract), and the
`PARENT_PATH` implementation of 3.22.1 / 3.22.2 is broken enough that another project refuses those versions
outright (zephyrproject-rtos/zephyr PR #43147, "test CMake version and issue error if using 3.22.1 / 3.22.2
from PyPI", which names the `cmake_path(... PARENT_PATH)` implementation as the bug).

## Commands and results

Configuration (all optional tools and the Demos off, so the tree configures on a plain host):

```
cmake -S . -B build-fh -DCMAKE_BUILD_TYPE=Release \
  -DOSMSCOUT_BUILD_TOOL_OSMSCOUT2=OFF -DOSMSCOUT_BUILD_TOOL_OSMSCOUTOPENGL=OFF \
  -DOSMSCOUT_BUILD_DEMOS=OFF -DOSMSCOUT_BUILD_TOOL_STYLEEDITOR=OFF \
  -DOSMSCOUT_BUILD_BINDING_JAVA=OFF -DOSMSCOUT_BUILD_MAP_OPENGL=OFF
```

Result: `-- Configuring done`, `-- Generating done`, exit 0.

Rendered headers (every enabled module at its include path):

```
find build-fh -name '*Features.h' | sort
```

Result — eleven headers, each at `<build>/<module>/include/<includedir>/`:

```
build-fh/libosmscout-client/include/osmscoutclient/ClientFeatures.h
build-fh/libosmscout-client-qt/include/osmscoutclientqt/ClientQtFeatures.h
build-fh/libosmscout-gpx/include/osmscoutgpx/GPXFeatures.h
build-fh/libosmscout-import/include/osmscoutimport/ImportFeatures.h
build-fh/libosmscout/include/osmscout/lib/CoreFeatures.h
build-fh/libosmscout-map-cairo/include/osmscoutmapcairo/MapCairoFeatures.h
build-fh/libosmscout-map/include/osmscoutmap/MapFeatures.h
build-fh/libosmscout-map-qt/include/osmscoutmapqt/MapQtFeatures.h
build-fh/libosmscout-map-skia/include/osmscoutmapskia/MapSkiaFeatures.h
build-fh/libosmscout-map-svg/include/osmscoutmapsvg/MapSVGFeatures.h
build-fh/libosmscout-test/include/osmscout-test/TestFeatures.h
```

No wrong path:

```
find build-fh -name '*Features.h.cmake*'
```

Result: no output — no directory named `<Name>Features.h.cmake/` in the build tree.

The fragile form is gone from the macro:

```
grep -n "PARENT_PATH" cmake/ProjectConfig.cmake
```

Result: no output.

Core library compiles against the rendered header named in its `HEADER` list:

```
cmake --build build-fh --target OSMScout -j 8
```

Result: `[100%] Built target OSMScout`.

Install places the header (the core library's own install script, since a whole-tree `cmake --install` also
wants every other target's artefacts built):

```
cmake -DCMAKE_INSTALL_PREFIX=<build>/prefix2 -DCMAKE_INSTALL_CONFIG_NAME=Release -P build-fh/libosmscout/cmake_install.cmake
```

Result: `-- Installing: …/prefix2/include/osmscout/lib/CoreFeatures.h`; the installed file carries the
configured gates (`OSMSCOUT_HAVE_LIB_MARISA` defined, `OSMSCOUT_HAVE_SSE2` / `OSMSCOUT_DEBUG_ROUTING` not).

## Test suite

The change touches no compiled source; the whole tree was still built and the suite run:

```
cmake --build build-fh -j 12
cd build-fh && QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR=<src>/Tests TESTS_TMP_DIR=<build>/Tests ctest -j 4 --output-on-failure
```

Result: `100% tests passed out of 156`, `Total Test time (real) = 35.25 sec`.

## Not run

- A build with the CMake 3.22 that shows the wrong directory — that CMake is not on this host (see above).
- The Android build that first surfaced the defect (`Android/`, Gradle-driven) — it needs the Android SDK/NDK,
  which is outside this repository and not configured here.
