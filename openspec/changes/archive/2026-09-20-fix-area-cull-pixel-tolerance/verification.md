# Verification

## 1. Baseline (task 1.1)

`Tests/PerformanceTest` built with Meson (`build-meson`, `debugoptimized`), rendered with
`--driver cairo --start-zoom 15 --end-zoom 15 --draw-repeat 5 --load-repeat 1 maps/Dortmund
stylesheets/standard.oss 51.514 7.463 51.510 7.470` (DPI default 96), pre-change build:

| run | loaded areas | loaded ways | step #4 min | avg | max | allocs |
|---|---|---|---|---|---|---|
| 1 (first run after a full rebuild) | 17629 | 6308 | 5.28 ms | 14.31 ms | 22.24 ms | 158 |
| 2 (warm repeat) | 17629 | 6308 | 1.88 ms | 2.12 ms | 2.48 ms | 158 |
| 3 (warm repeat) | 17629 | 6308 | 1.86 ms | 1.99 ms | 2.36 ms | 158 |

Run 1 is a cold-cache outlier (the same 3.7x class of variance the culling change documented), so the warm
repeats are the numbers to compare against. Tolerance the pre-change code applies to the widest area border
the frame resolves (0.1 mm per the culling change's measurement of the largest per-ring offset, 0.050 px):
**0.05** in both call sites - the raw millimetre value used as a pixel offset.

## 2. Caller audit (task 1.2)

`MapPainter::IsVisibleArea` / `IsVisibleWay` enlarge a **screen box by `pixelOffset` pixels**
(`MapPainter.cpp`, `IsVisibleArea`) and compare against `areaMinDimension`, which is itself set from
`projection.ConvertWidthToPixel(parameter.GetAreaMinDimensionMM())` (`MapPainter.cpp:2302`).

Callers that already pass screen lengths:

| site | offset | unit |
|---|---|---|
| `MapPainter.cpp:362` (`IsPointWayVisible`) | `databaseCache[dbIndex].wayReachPixel/2.0 + additionalOffsetPixel` | px (`wayReachPixel` is built from `GetProjectedWidth` / `ConvertWidthToPixel`, `MapPainter.cpp:313-319`) |
| `MapPainter.cpp:1038` (shield label decision) | `shieldLabelExtent + ShieldBorderInset + ShieldBackgroundClearance + GetLabelLayoutMarginPixel` | px |
| `MapPainter.cpp:1805` (way line styles) | `CalculateLineWith(...)/2` | px (`CalculateLineWith` returns `GetProjectedWidth` + `ConvertWidthToPixel`, `MapPainter.cpp:1626-1653`) |

Callers that pass a millimetre value as pixels (the defect):

| site | offset | unit |
|---|---|---|
| `MapPainter::PrepareAreaRing` (per-ring decision) | `borderStyle->GetWidth()/2.0` | mm - converted by this change |
| `MapPainter::ProcessAreas` (early decision) | `GetMaxAreaBorderWidthMM(level)*0.5` | mm - converted by this change |
| `MapPainterOpenGL::ProcessAreas` (`MapPainterOpenGL.cpp:299-303`) | `borderWidth/2.0` | mm - left to the OpenGL backend, recorded in `TODO.md` |

## 3. Conversion in the painter (tasks 2.1 - 2.4)

- `PrepareAreaRing` passes `projection.ConvertWidthToPixel(borderWidth/2.0)`.
- `ProcessAreas` computes `maxAreaBorderWidthMM` and passes
  `projection.ConvertWidthToPixel(maxAreaBorderWidthMM*borderWidthToTolerance)`, i.e. the converted form of
  the per-ring expression (`borderWidthToTolerance == 0.5`), so the early offset is never smaller than a
  per-ring offset for any DPI. The debug assert still compares the two widths in millimetres, before the
  conversion.
- `MapPainter::IsVisibleArea` and `MapPainter::IsVisibleWay` carry a doc comment naming the pixel unit and
  the conversion (0 Uncrustify findings on the added lines).

## 4. Tests (tasks 3.1 - 3.3)

Probe: both conversions reverted in `MapPainter.cpp` (`TEMP-PROBE`, not committed), rebuilt, and the test
run - **5 test cases, 2 failed**:

```
The border tolerance of a stylesheet width follows the DPI of the projection
  REQUIRE( preparedAt(projection300)==1 )  0 == 1   (tolerance 18.8976 px at 96 DPI, 59.0551 px at 300 DPI,
                                                     area reaches the view by 19.8976 px)
An area within the border tolerance is not rejected
  0 == 1   (border width 100 mm, per-ring tolerance 590.551 px, raw value 50)
```

With the conversions in place: `All tests passed (498 assertions in 5 test cases)`.

Other assumptions checked (task 3.3): `MapPainterAreaPreparationTest` already passes
`projection.ConvertWidthToPixel(parameter.GetOptimizeErrorToleranceMm())`; the way, point object and label
culling tests pass unchanged (`743`, `3103` and `1057` assertions), `StyleConfigVisibilityBoundsTest` passes
(`25` assertions).

## 5. Builds (tasks 4.1 - 4.2)

- CMake (`build-cmake`, Debug, tools/demos/bindings off): full build exit 0, 6 warnings in the whole
  project, none in `MapPainter.cpp`, `MapPainter.h` or the test file (the warnings are the vendored
  GoogleTest `goto error` one, a marisa enum conversion, two Qt deprecations, a Qt `touchPoints`
  deprecation and an unused variable in `MapPainterOpenGL.cpp`).
- Meson (`build-meson`): compilation of the changed library and test targets without warnings.

## 6. Test suites (task 4.3)

| suite | result |
|---|---|
| CMake `ctest --exclude-regex PerformanceTest` (`xvfb-run`, `-j 4`) | 86/86 passed |
| CMake `ctest --tests-regex PerformanceTest` (`-j 2`) | 39/39 passed (includes `PerformanceTest-cairo-standard.oss`) |
| CMake `ctest -R "MapPainter\|StyleConfigVisibilityBounds"` | 10/10 passed |
| Meson `meson test` (all 125 tests) | 125/125 passed |

## 7. After-state (task 4.4)

Same fixed view and command as 1.1, after-change build:

| run | loaded areas | loaded ways | step #4 min | avg | max | allocs |
|---|---|---|---|---|---|---|
| 1 | 17629 | 6308 | 1.80 ms | 1.97 ms | 2.11 ms | 158 |
| 2 | 17629 | 6308 | 2.07 ms | 2.45 ms | 2.92 ms | 158 |
| 3 | 17629 | 6308 | 2.38 ms | 3.08 ms | 4.68 ms | 158 |

Loaded and prepared counts are identical to the baseline and the step allocation count is unchanged (158).
The step time is within the run-to-run variance of this machine (baseline warm 1.86 - 1.88 ms against
after-change 1.80 - 2.38 ms), as expected: for `stylesheets/standard.oss` the widest area border is 0.1 mm,
so the applied tolerance grows from 0.05 to
`ConvertWidthToPixel(0.05 mm) = 0.189` px at 96 DPI - a band of 0.139 px, which changes no decision on this
view. The visible effect of the change is confined to wide borders and high DPI, which the two-DPI unit
test pins.

## 8. Linters (task 4.5)

- Uncrustify: the test file is clean (0 hunks); `MapPainter.cpp` (93 hunks) and `MapPainter.h` carry
  pre-existing drift only - none of the hunks contains an added line of this change (checked by filtering
  the diff for the added identifiers and text).
- clang-tidy (`-p build-meson`): the test file is at 51 findings, none of them in the new test case or in
  `MakeProjectionWithDpi`; while the new lines still used literals it reported 56, i.e. the 5 findings the
  new code added (a narrowing conversion and the DPI/level magic numbers) were removed by an explicit
  `static_cast<double>` and by named constants.
- clang-tidy `MapPainter.cpp`: 456 findings for the file, all of classes the file already carries (magic
  numbers, `assert` as `static_assert`, unchecked container access). The only finding on a changed line is
  the magic number `2.0`, which the line contained before the change as well (`borderWidth/2.0` becomes
  `ConvertWidthToPixel(borderWidth/2.0)`).

## 9. TODO.md (task 4.6)

The entry "Millimetre border width used as a pixel offset in the per-ring visibility decision" was removed;
the OpenGL entry now records that `MapPainterOpenGL::ProcessAreas`' own visibility call still passes
`borderWidth/2.0` (millimetres) as the pixel offset.
