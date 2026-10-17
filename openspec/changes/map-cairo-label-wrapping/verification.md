# Verification

Branch `map-cairo-label-wrapping`, based on `origin/master`. Everything below was run on this
checkout on the machine described. The three source files match the reference commit `292fce729`
of `origin/naviveylin-local` hunk for hunk (`git diff 292fce729 -- <files>` is empty).

## 1. CMake build WITH Pango (host `build/`)

The host `build/` has Pango (1.58.2), so the two new host cases are compiled out there; this run
verifies the touched files compile in the Pango configuration and that nothing regresses.

```
cmake --build build --target TextMetricsCairoTest -j 4
ninja: no work to do.            # second invocation, after the build finished
./build/Tests/TextMetricsCairoTest
===============================================================================
All tests passed (152 assertions in 5 test cases)
```

## 2. CMake build WITHOUT Pango (`build-nopango/`)

Configured a second build with the text stack disabled and verified from the generated feature
header that `OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO` is undefined:

```
cmake -B build-nopango -DCMAKE_BUILD_TYPE=Debug -DCMAKE_DISABLE_FIND_PACKAGE_Pango=TRUE \
      -DOSMSCOUT_BUILD_DEMOS=OFF -DOSMSCOUT_BUILD_TOOL_OSMSCOUT2=OFF \
      -DOSMSCOUT_BUILD_TOOL_OSMSCOUTOPENGL=OFF -DOSMSCOUT_BUILD_TOOL_STYLEEDITOR=OFF \
      -DOSMSCOUT_BUILD_BINDING_JAVA=OFF -DCMAKE_UNITY_BUILD=OFF
grep -n "PANGO" build-nopango/libosmscout-map-cairo/include/osmscoutmapcairo/MapCairoFeatures.h
4:#ifndef OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO
6:/* #undef OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO */

cmake --build build-nopango --target TextMetricsCairoTest -j 4
...
[100%] Building CXX object Tests/CMakeFiles/TextMetricsCairoTest.dir/src/TextMetricsCairoTest.cpp.o
Linking CXX executable TextMetricsCairoTest
Built target TextMetricsCairoTest

./build-nopango/Tests/TextMetricsCairoTest --list-tests
  ... 7 test cases, including the two new ones:
  Cairo wraps a multi-word label into the proposed width
  Cairo never breaks a word and keeps a short label on one line

./build-nopango/Tests/TextMetricsCairoTest
===============================================================================
All tests passed (167 assertions in 7 test cases)
```

The two new cases therefore run and pass in the configuration the unit targets (5 cases become 7
when the `!defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)` guard is active).

## 3. Meson build (`build-meson-check/`)

The existing Meson build directory was used as-is (its auto-reconfigure ran because sources
changed). It has Pango, so the new host cases are compiled out; the run verifies the touched files
compile in Meson too.

```
meson compile -C build-meson-check TextMetricsCairoTest
...
[107/116] Compiling C++ object libosmscout-map-cairo/libosmscout_map_cairo.so.1.1.1.p/src_osmscoutmapcairo_MapPainterCairo.cpp.o
[115/116] Linking target Tests/TextMetricsCairoTest

./build-meson-check/Tests/TextMetricsCairoTest
===============================================================================
All tests passed (152 assertions in 5 test cases)
```

## 4. Label / text-metrics regression tests (CMake, Pango)

Rebuilt the targets that touch the changed label struct / measurement and ran them:

```
cmake --build build --target FeatureLabelTest LabelLayouterTest LabelPathTest \
      MapPainterLabelCullingTest MapPainterLabelReuseTest TextMetricsTest \
      TextMetricsSVGTest TextMetricsReferenceTest TextMetricsCairoTest -j 4
cd build && ctest -R "FeatureLabelTest|LabelLayouterTest|LabelPathTest|MapPainterLabelCullingTest|MapPainterLabelReuseTest|TextMetricsTest|TextMetricsCairoTest|TextMetricsSVGTest|TextMetricsReferenceTest" --output-on-failure
100% tests passed, 0 tests failed out of 9
```

The cross-backend measurement cases (`TextMetricsTest`, `TextMetricsReferenceTest`,
`TextMetricsSVGTest`) state no wrapping and are unchanged.

## 5. OpenSpec validation

```
openspec validate map-cairo-label-wrapping --strict
Change 'map-cairo-label-wrapping' is valid
```

## 6. Pango variant untouched

The Pango-guarded branches of `Layout`, `MeasureText` and `DrawLabel` (the `#if
defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)` regions) are byte-for-byte identical to `origin/master`;
all new code sits in the `#else` region or after the `#if/#else/#endif` block. Every change matches
the reference commit `292fce729`. The Pango configuration builds and its test binary passes (see
sections 1 and 3).

## Not run, and why

- **Full `ctest` suite / ASan build.** Out of scope for this unit and not required by the task:
  only the `TextMetricsCairoTest` target and the label/text-metrics targets were built and run, per
  the environment rules (a bare `cmake --build build` builds Demos, Qt apps and `PerformanceTest`).
- **Rendering against a real database.** `maps/` holds v26 databases while the library expects v27,
  and there is no routable database on this machine; this unit needs no database.
- **`scripts/format-check.sh`.** Red repo-wide on this machine, including pristine master files; not
  used as a gate. Style was matched by hand.
