# Verification: map-painter-area-visibility-cull

Evidence collected while applying the tasks in `tasks.md`.

## Environment

- Build: `build/` (CMake, Ninja, `CMAKE_BUILD_TYPE=Release`), map backends Cairo, Qt, Skia, SVG,
  OpenGL enabled. `debug/` (CMake, Ninja, `-O2 -g`, no `NDEBUG`, so the debug assertions are live)
  and `build-asan/` (configured as in AGENTS.md, `CMAKE_UNITY_BUILD=ON`) as needed.
- Database: `maps/Dortmund` (508 types), stylesheet `stylesheets/standard.oss`, icons
  `libosmscout/data/icons`.
- Fixed view: one tile at zoom 15 over the Dortmund city centre, the same view the
  `map-painter-area-preparation` and `map-painter-frame-containers` baselines used.

```
./build/Tests/PerformanceTest \
  --driver cairo --start-zoom 15 --end-zoom 15 \
  --draw-repeat 5 --load-repeat 1 --icons libosmscout/data/icons \
  maps/Dortmund stylesheets/standard.oss 51.514 7.463 51.510 7.470
```

### Loaded and prepared data of that view

| quantity | value | source |
|---|---|---|
| loaded per frame | 164 nodes, 6308 ways, 17629 areas | `Tot. data` of the run |
| prepared area entries | 726 | `Draw areas: 726 (pcs)`, temporary debug-performance probe |
| prepared ways | 335 | `Draw ways: 335 (pcs)`, temporary debug-performance probe |
| rings reaching per-ring preparation | 17650 | temporary probe, see section 2.2 |
| rings taking part in the frame | 726 (4.1 %) | temporary probe, see section 2.2 |

All 17650 rings reach per-ring style resolution and only 726 of them take part in the frame. This is
the waste the change removes.

### Measurement stability

Wall time on this machine is not stable. Repeated runs of the identical command and view reported

```
#4 ProcessAreas min   3.68 .. 13.52 ms      (3.7x spread, measured over this work)
Map            avg  32.11 .. 38.31 ms
```

so **only comparisons made inside a single interleaved run are used**, and the minimum is reported
next to the average. `load average` was 4 - 10 during the measurements (16 cores).

## 1.1 Baseline before the change

Clean master build, 5 draw repetitions, one level:

```
 Tot. data  : nodes: 164 way: 6308 areas: 17629
 Draw allocs: total: 23188 avg: 4637
 Map        : total: 191.53 min: 33.36 avg: 38.31 max: 48.48
               #4 14% total: 26.93 min: 5.05 avg: 5.39 max: 5.75 allocs: 161 (1%)
               #11 29% total: 55.87 min: 10.78 avg: 11.17 max: 11.66 allocs: 24 (0%)
               #12 8% total: 15.22 min: 2.91 avg: 3.04 max: 3.19 allocs: 20 (0%)
```

| metric | before |
|---|---|
| step 4 `ProcessAreas`, ms/frame | min 5.05, avg 5.39 |
| step 4 allocations per frame | 161 |
| frame allocations per frame | 4637 |
| frame total, avg ms | 38.31 |

Three consecutive runs of the same command, step 4 only:

```
min 4.81 avg 5.06
min 5.04 avg 5.26
min 4.99 avg 5.42
```

## 2.1 The bound of the loaded style sheet

`StyleConfig::GetMaxAreaBorderWidthMM` is filled by `PostprocessAreas` from the built per-level border
style selector tables (so it covers exactly the styles the per-ring decision can read) and is documented
in the header. Measured by the unit test
`The border width bound is the widest border style of a level`, with border styles of 1.0 mm at the
levels 0-9, 2.0 mm at 10-14 and 0.5 mm at 15-19:

```
bound at level 5  : 1.0
bound at level 12 : 2.0
bound at level 17 : 0.5
```

## 2.2 The bound covers every tolerance the per-ring decision can use

Temporary probe (`TEMP-PROBE`, reverted) recording the largest `borderWidth/2.0` reached by
`PrepareAreaRing` in a frame next to the bound of the early decision, on the fixed view:

```
PROBE bound=0.5000 maxPerRingTolerance=0.0500 covered=yes
PROBE bound=0.5000 maxPerRingTolerance=0.0500 covered=yes
PROBE bound=0.5000 maxPerRingTolerance=0.0500 covered=yes
```

The widest area border style the loaded style sheet can resolve at level 15 is 1.0 mm, so the bound is
0.5, while the widest border actually used by a ring that reaches the test is 0.1 mm, i.e. 0.05. The
bound therefore covers every per-ring tolerance used, with room to spare.

## 3.1 The early decision in the area preparation

`MapPainter::ProcessAreas` rejects an area ahead of both `PrepareArea` call sites (`mapData.areas` and
`mapData.poiAreas`) using `IsVisibleArea` on the area's bounding box with the bound of 2.1 as its
tolerance. Verified on the fixed view with the temporary debug-performance probe, once with the early
decision active and once with it disabled, the two libraries swapped through `LD_LIBRARY_PATH`:

```
guard ON  (after)   Draw areas: 726 (pcs)   Draw ways: 335 (pcs)
guard OFF (before)  Draw areas: 726 (pcs)   Draw ways: 335 (pcs)
```

The prepared area entries and the prepared ways are identical, so the early decision does not change what
is prepared. The build is warning-free for the changed files (see 5.1).

## 3.2 The invariant in the per-ring path

`PrepareAreaRing` asserts that the per-ring tolerance never exceeds the derived bound. Verified in the
`debug/` tree (assertions live):

- with the bound as implemented, the assert does not fire for the whole test suite;
- with the bound deliberately reduced by a factor of 20 in a scratch build, it fires on the fixed view:

```
PerformanceTest: ../libosmscout-map/src/osmscoutmap/MapPainter.cpp:1193:
  bool osmscout::MapPainter::PrepareAreaRing(...):
  Assertion `borderWidth<=styleConfig.GetMaxAreaBorderWidthMM(projection.GetMagnification())/20.0' failed.
```

The assertion is not reachable from user input: both sides are border widths of the same loaded style
sheet, so only a logic error in deriving the bound can violate it.

## 4.1 - 4.4 The unit tests

`Tests/src/MapPainterAreaVisibilityCullTest.cpp`, 4 test cases, 433 assertions, registered in
`Tests/CMakeLists.txt` and `Tests/meson.build`.

| task | test | requirement |
|---|---|---|
| 4.1 | `Areas outside the view are not prepared ring by ring` | Preparation work follows the visible areas, not the loaded ones |
| 4.2 | `An area within the border tolerance is not rejected` | The early rejection is conservative |
| 4.3 | `Prepared entries and clipping geometry do not depend on the loaded areas` | Prepared entries, clipping geometry, orders and rendered output are unchanged |
| 2.1 | `The border width bound is the widest border style of a level` | The early rejection is conservative |
| 4.5 | manual PNG comparison, see below | Prepared entries, clipping geometry, orders and rendered output are unchanged |

The work contract of 4.1 is observed through `MapParameter::RegisterFillStyleProcessor`: a counting
`FillStyleProcessor` is installed for the styled type, and the test asserts that per-ring style
resolution is reached once (the one area inside the view) and not at all for the 64 loaded areas
outside it. 4.2 runs the same three cases - inside the view, just outside it within the per-ring
tolerance, and far outside any tolerance - for two style sheets whose border widths differ by a factor
of 1000, asserts that the derived bound grows with the style sheet and that it is never smaller than
that style sheet's per-ring tolerance, and derives both from the same `borderWidthToTolerance` constant
the painter uses. 4.3 compares the prepared frame of a view with and without 128 further loaded areas
outside the view: the prepared area entries by type, role, transformed coordinates, clipping ranges and
draw order, and the number of prepared ways, with a check that the compared view does prepare a way and
a check that a view loading only areas outside the viewport prepares no area but still prepares its way.

## 4.5 The rendered output (manual comparison)

The spec's two rendered-output scenarios (*Rendered output is unchanged* and *Clipping geometry is
unchanged* -> the rendered output shows the same clipping) are verified by rendering the same view with
and without the early decision and comparing the images byte for byte, using the console demo that
writes a PNG (`Demos/src/DrawMapCairo.cpp`) with the two libraries swapped through `LD_LIBRARY_PATH`:

```
DrawMapCairo --database maps/Dortmund --iconPath libosmscout/data/icons --iconMode Scalable \
  --width 400 --height 400 --fontName <a font of this machine> \
  stylesheets/standard.oss 51.512 7.4665 70000 /tmp/view-<variant>.png
```

`70000` is a magnification value, not a level; the run reports the view it draws as
`[51.50950 N 7.46248 E - 51.51450 N 7.47052 E] 70000x/16 400x400`, i.e. the city centre at level 16, the
same area as the fixed view above.

| variant | exit | bytes | md5 |
|---|---|---|---|
| with the early decision | 0 | 175654 | `1c0e2c0816b9f4909738560063240048` |
| without the early decision | 0 | 175654 | `1c0e2c0816b9f4909738560063240048` |

`cmp` reports no difference. The 175654 bytes and 400x400 dimensions confirm the image is a full city
render and not a blank canvas, so the comparison is not vacuous. This is a manual check, not an
automated test: there is no useful automated rendering comparison here, because the property being
verified is that the pixels did not move, not that they match a new expectation.

## 4.6 Sensitivity of the tests

Checked by building against deliberately broken variants of the library:

| variant | observed |
|---|---|
| early decision disabled (both call sites) | `Areas outside the view are not prepared ring by ring` fails: `REQUIRE(countingProcessor->invocations==1)` -> `65 == 1` |
| early bound set to 0 (deliberately aggressive) | `An area within the border tolerance is not rejected` fails: `REQUIRE(preparedWithLeftEdgeAt(screenRight+perRingTolerancePx-10.0)==1)` -> `0 == 1` |
| bound in the per-ring path reduced by 20 | the assert of 3.2 fires (see there) |

Deviation from the plan: task 4.3's verification expected the "deliberately aggressive bound" case to
fail *that* test. It does not, and it cannot: the compared areas lie inside the viewport, so both the
reference and the culled frame lose them together and the comparison stays equal. The aggressive-bound
case is therefore detected by 4.2 (it is the test that places an area inside the per-ring tolerance) and
by the assertion of 3.2. 4.3's own sensitivity is the "only areas outside the viewport prepare no area"
comparison and the entry and way comparisons: it fails if the early decision changes what is prepared.

## 4.7 Review findings and their resolution

A verification pass over this change raised four warnings and three suggestions. All were resolved:

| finding | resolution |
|---|---|
| no automated coverage for the prepared-way count | the way assertions of 4.3 above; the compared view now prepares a way (`reference.ways==1`) and the count is required to be identical with 128 further loaded areas |
| rendered-output scenarios verified structurally only | the manual PNG comparison of 4.5, byte-identical with and without the early decision |
| proposal claimed "No public API change" | corrected: the proposal now names the one added accessor on `StyleConfig` |
| proposal described the bound as being "in pixels" | corrected to the unit the per-ring decision uses, matching design D2 |
| proposal listed `include/osmscoutmap/MapPainter.h` as affected | removed; that header was not modified |
| the test re-derived the halving as `/2.0` | both the painter and the test now use a named `borderWidthToTolerance` constant |
| the bound's growth across style sheets was not asserted | 4.2 now asserts `bound > previousBound` across the 0.1 mm and 100 mm style sheets |

## 5.1 - 5.4 Builds and existing tests

| check | command | result |
|---|---|---|
| 5.1 | `cmake --build build` (Release, all targets) | exit 0, 3 s; no warning from any changed file |
| 5.2 | `meson compile -C build-meson` | exit 0, 16 s; no warning from any changed file |
| 5.3 | `ctest -j 2` in `build/` | 100 % tests passed out of 118, 32 s |
| 5.4 | `meson test -C build-meson` | Ok: 71, Fail: 0, 51 s |

Environment used for the test runs:
`QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR=<source>/Tests TESTS_TMP_DIR=<build>/Tests`.

The only warnings in either build log are pre-existing and in files this change does not touch:
vendored `nanosvg.h` in the Skia backend, the Doxygen configuration, `MapPainterOpenGL.cpp:464`, the
CMake find-module author warnings, and a Javadoc warning in `libosmscout-client-java`.

## 5.5 After measurement against the baseline

Two libraries built from the same source, one with the early decision and one without, run alternately
through `LD_LIBRARY_PATH` so both are measured under the same load. Step 4 `ProcessAreas`, ms/frame:

```
run 1 (lighter load)            min     avg            run 2 (heavier load)          min     avg
before (no early decision)      3.39    3.56           before                        7.16    9.36
after  (early decision)         1.95    2.19           after                         5.07    5.43
before                          3.94    4.08           before                        7.27    7.51
after                           2.09    2.40           after                         5.14    6.33
before                          4.27    4.97           before                        7.16    9.54
after                           2.46    2.59           after                         4.83    5.86
before                          4.12    5.05           before                        6.97    7.50
after                           2.40    2.52           after                         5.02    6.40
before                          3.97    4.27
after                           2.14    2.39
```

| | minimum | mean of averages |
|---|---|---|
| run 1 before / after | 3.39 / 1.95 (-42 %) | 4.39 / 2.42 (-45 %) |
| run 2 before / after | 6.97 / 4.83 (-31 %) | 8.48 / 6.01 (-29 %) |

The two sets do not overlap in either run, so the step is reduced by **29 % - 45 %** depending on how
loaded the machine is, i.e. by about 1.4 - 2.1 ms of a step that is roughly 14 % of a frame.

Prepared counts and allocations are unchanged by the change, as expected for work that is removed and
not converted: `Draw areas: 726`, `Draw ways: 335` and 158 - 161 allocations per frame for step 4 both
with and without the early decision, against 4637 allocations per frame for the whole frame in the
baseline. The change does not reduce allocations, it reduces work.

## 5.6 Linters

Uncrustify 0.83.0_f with the project `.uncrustify`:

| file | drift at HEAD | drift after | added lines flagged |
|---|---|---|---|
| `libosmscout-map/src/osmscoutmap/MapPainter.cpp` | 1354 | 1355 | 0 |
| `libosmscout-map/include/osmscoutmap/StyleConfig.h` | 522 | 527 | 0 |
| `libosmscout-map/src/osmscoutmap/StyleConfig.cpp` | 0 | 0 | 0 |
| `Tests/src/MapPainterAreaVisibilityCullTest.cpp` | (new) | 0 | 0 |

The new file was brought to zero drift; the drift in the two modified library files is pre-existing
(uncrustify reports drift in both at HEAD) and no added line is part of it. Where an added line was one
uncrustify rewrites, the tool's form was adopted for that line: `double & maxWidth` in `StyleConfig.cpp`
(right-attached references, as `.uncrustify` wants) and the wrapped `earlyOffset` declaration in
`MapPainter.cpp`. `guidelines/CodeStyles.md` documents left-attached references, so the two sources
disagree and the changed lines follow the tool, as the earlier painter changes did.

clang-tidy (`.clang-tidy`, `clang-tidy -p build`), findings restricted to the lines this change adds:

| file | added lines | findings in added lines |
|---|---|---|
| `libosmscout-map/src/osmscoutmap/MapPainter.cpp` | 29 | 0 |
| `libosmscout-map/include/osmscoutmap/StyleConfig.h` | 21 | 0 |
| `libosmscout-map/src/osmscoutmap/StyleConfig.cpp` | 40 | 0 |
| `Tests/src/MapPainterAreaVisibilityCullTest.cpp` | all (new file) | 45 findings |

The findings reported in the modified library files are pre-existing ones in the surrounding code and
in the headers those files include (for example `CoordBufferRange` in `Transformation.h`). The new test
file carries 58 findings, all in categories the neighbouring painter tests already carry
(`MapPainterFrameBuffersTest.cpp` carries 91 findings and `MapPainterAreaPreparationTest.cpp` 130;
`modernize-use-emplace` 10 here against 7 and 9 there; `modernize-use-designated-initializers` 1 here
against 2 and 1 there), and it introduces no check category the neighbouring files do not carry. The
`readability-math-missing-parentheses` and `cppcoreguidelines-pro-type-member-init` findings it started
with were removed, and its `operator[]` accesses were replaced by `.at()`.

## 5.7 TODO.md

Three findings of this work that are not part of this change are recorded in `TODO.md` under
"Pre-existing issues found during implementation of `map-painter-area-visibility-cull`": the area
preparation early-out that no shipped style sheet reaches, the millimetre border width used as a pixel
offset in the per-ring visibility decision (the reason design decision D2 does not convert the bound),
and the OpenGL backend's per-data-load reprocessing of all loaded areas including its quadratic
duplicate point removal ahead of the visibility test.

## 5.8 Sanitizer build

`build-asan/` configured as documented in AGENTS.md (`Debug`, `-fsanitize=address -fsanitize=undefined`,
`-DCMAKE_UNITY_BUILD=ON`, tool/demo/java targets off), full build, then

```
ctest -j 2 --output-on-failure --exclude-regex "PerformanceTest"
```

result: **96 % tests passed, 3 tests failed out of 76**.

| failing test | cause |
|---|---|
| `MapPainterShieldTest` | `LeakSanitizer: detected memory leaks`, 65536 bytes in 256 objects |
| `TextMetricsCairoTest` | same class of report |
| `TextMetricsSVGTest` | same class of report |

These are not regressions of this change:

- the three tests report `All tests passed` for their assertions and fail only on the leak check;
- every allocation frame of the report is inside `libfontconfig.so.1` (`FcFontSetList`) or
  `libpango-1.0.so.0`, reached from `MapPainterCairo::Layout`; the only libosmscout frames are the
  unmodified text-layout caller and the test that calls it, and no frame is in code this change touches;
- with `ASAN_OPTIONS=detect_leaks=0` the same four tests, including the new one, pass
  (`100 % tests passed out of 4`).

This is the same class of failure AGENTS.md already documents for `PerformanceTest` ("UI libraries leak
on exit"). Limitation: no pre-change sanitizer build was produced for comparison, so the pre-existence
of these three leak reports rests on the report contents above rather than on an A/B run.

`MapPainterAreaVisibilityCullTest` passes in the sanitizer build.
