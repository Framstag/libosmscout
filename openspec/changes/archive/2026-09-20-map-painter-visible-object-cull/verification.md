# Verification: map-painter-visible-object-cull

Evidence collected while applying the tasks in `tasks.md`.

## Environment

- Build: `build/` (CMake, Ninja, `CMAKE_BUILD_TYPE=Release`), map backends Cairo, Qt, Skia,
  SVG, OpenGL enabled; an assertions-enabled tree at `/tmp/lc-assert` (CMake, Ninja,
  `CMAKE_BUILD_TYPE=Debug`, map backends Cairo and Qt only, demos and tools off) for the debug
  assertion of design D7.
- Database: `maps/Dortmund` (508 types), stylesheet `stylesheets/standard.oss`, icons
  `libosmscout/data/icons`, font `/usr/share/fonts/liberation/LiberationSans-Regular.ttf`.
- Fixed view: one tile at zoom 15 over the Dortmund city centre
  (51.514 7.463 - 51.510 7.470), the same view the previous painter changes used. Loaded data of
  that view: 164 nodes, 6308 ways, 17629 areas; prepared for the frame: 335 ways and 726 areas.
- Measurement harness: two libraries built from the same source (one with the change, one
  without) swapped through `LD_LIBRARY_PATH`, as the area work established, because wall time on
  this machine is not stable (load average 4 - 10 during the work). Allocation counts are exact
  and reproducible and are used as the anchor.

## 1. Unit tests

| file | cases | capability |
|---|---|---|
| `Tests/src/StyleConfigVisibilityBoundsTest.cpp` | 4 | per-level visibility bounds (way line reach, icon and symbol reach, label extent bound) |
| `Tests/src/MapPainterWayCullingTest.cpp` | 4 | `map-painter-way-culling` |
| `Tests/src/MapPainterPointObjectCullingTest.cpp` | 4 | `map-painter-point-object-culling` |
| `Tests/src/MapPainterLabelCullingTest.cpp` | 7 | `map-painter-label-culling` |

Scenarios of the three capabilities, traced to their test case:

| scenario | test |
|---|---|
| way: a way outside the view contributes nothing | `A way outside the view is not prepared` |
| way: the rejection is never more aggressive than the per-line-style decision | `The way rejection follows the reach of the style sheet` |
| way: shield labels of a rejected way are not registered | `A shield styled way outside the view adds no labels` |
| way: additional loaded ways outside the view do not add work | `Loaded ways outside the view do not add work` |
| way: prepared ways, order and rendered output unchanged | `A way outside the view is not prepared` (coordinates), `MapPainterFrameBuffersTest`, `MapPainterRouteTest`, 6.6 (image) |
| point object: an object of a type without a label style contributes nothing | `Loaded point objects of an icon-only type add no work` |
| point object: an object whose labels cannot reach the view contributes nothing | `A point object outside the view registers no elements` |
| point object: an object inside the view is prepared as before | `A point object outside the view registers no elements` (the visible object), `A point object at the viewport edge keeps its elements` |
| point object: an object near the viewport edge keeps its elements | `A point object at the viewport edge keeps its elements` |
| point object: additional loaded objects do not add allocations | `Loaded point objects of an icon-only type add no work` |
| point object: the registered element set is unchanged | `Loaded point objects outside the view do not add work`, `Loaded point objects of an icon-only type add no work` |
| label: a label outside the view is not measured | `A label outside the view is not measured` |
| label: a label that reaches into the view is kept | `A label that reaches into the view is measured` |
| label: the same rule holds for every label source | `Icon and element list labels outside the view are not stored` (icon, symbol and element list), way and point object tests (shield and node labels) |
| label: off-view labels do not add measurements | `Off-view labels do not add measurements`, `The stored labels are the labels that can appear` |
| label: set, placement and measurement results unchanged | `A label that reaches into the view is measured` (placement), `The stored labels are the labels that can appear` (set), 6.6 (image) |
| label: labels of a frame without a known viewport are kept (design D9) | `Labels of a frame without a known viewport are kept` |
| label: labels in the layout margin are kept (design D9) | `Labels in the layout margin are kept` |

### Sensitivity of the tests

Checked by building against deliberately broken variants of the library:

| variant | observed |
|---|---|
| label decision disabled in both registration overloads | `A label outside the view is not measured` fails (`LayoutCalls()==0` -> `1`), `Off-view labels do not add measurements` fails (`0` -> `1000`) |
| point object decision disabled (both sites) | `Loaded point objects of an icon-only type add no work` fails (`0` -> `512` allocations of the further objects) |
| label extent bound reduced by a factor of 10 | the debug assertion `AssertElementsInsideReach` fires in the assertions-enabled tree (`LabelLayouter.h:389`, `element.x>=point.GetX()-reach`), and the label test fails |
| way rejection disabled in `CalculateWayPaths` | **no test fails**: the per-line-style visibility decision rejects the same ways, so the way rejection changes work, not the prepared set. Its effect is verified by the measurement of 5.1/5.2 (step `#2` -76 %), see the note in `TODO.md` about the missing line style observation hook. |

## 2. Debug assertion (design D7)

`AssertElementsInsideReach` (shared label layouter) checks that the rectangle of every element a
label built stays inside the anchor plus the reach of the label.

- Assertions-enabled tree (`/tmp/lc-assert`, `CMAKE_BUILD_TYPE=Debug`): the whole test suite
  passes, i.e. the assertion does not fire for the change. `MapPainterLabelCullingTest`: passed.
- With the label extent bound deliberately reduced by a factor of 10
  (`GetLabelExtentBound`, `LabelLayouterHelper.cpp`), the assertion fires:
  `MapPainterLabelCullingTest: LabelLayouter.h:389: ... Assertion 'element.x>=point.GetX()-reach' failed.`
  The reduction was reverted afterwards.
- A large real view (zoom 16, 51.505 7.455 - 51.519 7.478: 21212 nodes, 143504 ways, 151668
  areas) ran without the assertion firing in the assertions-enabled tree.

## 3. Before/after measurement

Fixed view, Cairo backend, 5 frames per run, the two libraries swapped through
`LD_LIBRARY_PATH` (3 interleaved rounds; the table shows the minimum and the mean of the
average of the rounds, `allocs` are per run of 5 frames):

| step | before, ms/frame | after, ms/frame | allocations per run |
|---|---|---|---|
| `#2` CalculatePaths (way preparation) | 3.10 / 3.30 - 4.37 | 0.88 / 0.93 - 1.07 | 441 -> 31 (-93 %) |
| `#3` CalculateWayShields | 1.07 / 2.62 - 4.46 | 0.60 / 2.12 - 2.80 | 4299 -> 561 (-87 %) |
| `#18` PrepareNodeLabels | 0.41 / 0.64 - 0.65 | 0.37 / 0.65 - 0.72 | 3255 -> 1665 (-49 %) |
| `#22` DrawLabels | 1.85 / 2.04 - 2.33 | 1.75 / 2.02 - 2.10 | 10030 -> 3610 (-64 %) |
| frame, allocations per frame | 4637 | 2207 (-52 %) | |
| frame, average ms | 31.3 - 39.6 | 30.5 - 32.1 | |

Zoom band, same box, Cairo, 3 frames per run (per frame values); `#4` ProcessAreas is the step
the change does not touch and stayed within the measurement noise with identical allocations
(161 per run in both variants), i.e. no regression in an unrelated step:

| view | frame allocs/frame | `#2` ms (allocs) | `#3` ms (allocs) | `#18` ms (allocs) | `#22` ms (allocs) |
|---|---|---|---|---|---|
| z14 before | 5493 | 1.92 (97) | 2.86 (465) | 0.60 (576) | 0.85 (1216) |
| z14 after | 4078 | 0.60 (8) | 0.22 (46) | 0.29 (360) | 0.71 (523) |
| z16 before | 24345 | 2.59 (2669) | 1.40 (3043) | 7.01 (40289) | 5.95 (40871) |
| z16 after | 18118 | 0.61 (346) | 1.10 (377) | 5.98 (33894) | 5.33 (27344) |
| z17 before | 11777 | 2.10 (6790) | 1.87 (23347) | 2.96 (58462) | 4.18 (85309) |
| z17 after | 6207 | 0.45 (582) | 0.58 (772) | 3.15 (45131) | 4.31 (38294) |

Notes on the numbers:

- The way preparation step loses about three quarters of its time and 93 % of its allocations;
  the remaining allocations are the reused stores of the painter.
- The shield step keeps a large maximum in the before runs (7 - 16 ms) because it registers one
  label per grid position of every loaded shield styled way; the change removes that for ways
  outside the view.
- The node label and label drawing steps keep their per-object work for objects whose type has
  label styles (see the per-object label element store finding in `TODO.md`); their allocation
  share drops because off-view elements are no longer stored and measured.
- The gain is smaller than the loaded-versus-visible ratio of the objects (about 5 %) suggests,
  which is the coarse bound of design D3 at work: it is deliberately generous.

## 4. Rendered output

`Demos/DrawMapCairo`, the same view twice, once with the library of the change and once without,
the two libraries swapped through `LD_LIBRARY_PATH`:

```
DrawMapCairo --database maps/Dortmund --iconPath libosmscout/data/icons --iconMode Scalable \
  --width 400 --height 400 --fontName /usr/share/fonts/liberation/LiberationSans-Regular.ttf \
  stylesheets/standard.oss 51.512 7.4665 70000 /tmp/view-<variant>.png
```

| variant | bytes | md5 | differing pixels |
|---|---|---|---|
| without the change | 175654 | `1c0e2c0816b9f4909738560063240048` | - |
| with the change | 175654 | `1c0e2c0816b9f4909738560063240048` | 0 |

The view reports `[51.50950 N 7.46248 E - 51.51450 N 7.47052 E] 70000x/16 400x400`, i.e. the city
centre at level 16, and the 175654 bytes confirm a full city render, so the comparison is not
vacuous.

**Two corrections were needed to reach this** (design D9, both found by this comparison):

| variant of the change | differing pixels |
|---|---|
| first implementation, before the corrections | 810 (5 labels dropped, 4 labels appeared in their place) |
| label stage decision gated on the viewport of the current frame | 103 |
| additionally, the painter decisions leave room for the LAYOUT margin (layouter overlap plus label padding) | 0 |

The dropped labels were shield labels ("B 54" at the left and right edges of the tile) and point
labels ("Günter-Samtlebe-Platz", "Beckmanns Backlokal") that the drawing path draws; the labels
that appeared in their place were neighbours that the dropped labels had suppressed.

## 5. Builds, tests and linters

| check | command | result |
|---|---|---|
| 5.1 | `cmake --build build` (Release, all targets) | exit 0; no warning in a changed file (the remaining warnings are the pre-existing ones: `MapPainterOpenGL.cpp:464`, Doxygen tags, `nanosvg.h`, `MapWidget.cpp`) |
| 5.2 | `meson setup --reconfigure build-meson` + `meson compile -C build-meson <four new targets>` | exit 0, no warning |
| 5.3 | `ctest -j 2` in `build/` | 100 % tests passed out of 121 |
| 5.4 | `meson test -C build-meson` | Ok: 74, Fail: 0 |
| 5.5 | ctest in the assertions-enabled tree | `MapPainterLabelCullingTest` passed |
| 5.6 | Uncrustify 0.83.0_f with the project `.uncrustify` | the four new test files have 0 lines of drift; the changed library files have no added line flagged (see below) |
| 5.7 | `clang-tidy -p build` (LLVM 22.1.8) | no new finding category; the two new helpers of `StyleConfig.cpp` that had one were fixed (see below) |

Environment for the test runs: `QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR=<source>/Tests
TESTS_TMP_DIR=<build>/Tests`.

### Uncrustify drift

| file | drift at HEAD | drift after the change | added lines flagged |
|---|---|---|---|
| `libosmscout-map/src/osmscoutmap/StyleConfig.cpp` | 0 | 6 | 0 |
| `libosmscout-map/src/osmscoutmap/LabelLayouterHelper.cpp` | 5 | 5 | 0 |
| `libosmscout-map/src/osmscoutmap/MapPainter.cpp` | 521 | 528 | 0 |
| `libosmscout-map/include/osmscoutmap/StyleConfig.h` | 200 | 207 | 0 |
| `libosmscout-map/include/osmscoutmap/MapPainter.h` | 108 | 114 | 0 |
| `libosmscout-map/include/osmscoutmap/LabelLayouter.h` | 316 | 328 | 0 |
| `libosmscout-map/include/osmscoutmap/LabelLayouterHelper.h` | 45 | 49 | 0 |

The new test files were brought to zero drift with the project configuration. The drift of the
modified library files grows by the number of declaration blocks the change adds or touches,
because uncrustify realigns the neighbouring declarations of such a block; no changed line is
flagged itself (checked by comparing the lines the change adds against the lines uncrustify
rewrites). The affected blocks carry the pre-existing drift of these files (pointer and reference
attachment, brace style of `enum`, member alignment), which a file-wide formatting pass in a
dedicated change would resolve; `TODO.md` already records that drift for `MapPainterCairo.cpp`,
`MapPainterQt.cpp` and `MapPainterSkia.cpp`.

### clang-tidy

`clang-tidy -p build` (LLVM 22.1.8) on the new test files and on the changed source files:

| file | findings | note |
|---|---|---|
| `Tests/src/StyleConfigVisibilityBoundsTest.cpp` | 128 `cppcoreguidelines-avoid-magic-numbers`, 59 `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access`, 48 `readability-math-missing-parentheses`, 43 `cppcoreguidelines-special-member-functions`, ... | no category the existing tests of the same kind do not have: `Tests/src/MapPainterAreaPreparationTest.cpp` reports 182, 91, 70, 52, 20 (`bugprone-easily-swappable-parameters`) of the same categories |
| `Tests/src/MapPainterWayCullingTest.cpp`, `...PointObjectCullingTest.cpp`, `...LabelCullingTest.cpp` | the same categories at a comparable volume (134 - 145 magic numbers, 79 - 80 unchecked container access, ...) | as above; the findings come from the included library and Catch2 headers and from the test style the project uses |
| `libosmscout-map/src/osmscoutmap/StyleConfig.cpp` | 133 `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access`, 104 `avoid-magic-numbers`, 46 `misc-include-cleaner`, ... | the two new static helpers first added `misc-use-anonymous-namespace` (a category the file did not have); they were moved into an anonymous namespace, so the file has no finding of that category again |
| `libosmscout-map/src/osmscoutmap/LabelLayouterHelper.cpp` | 98 `avoid-magic-numbers`, 19 `bugprone-easily-swappable-parameters`, 11 `misc-include-cleaner` | `GetLabelExtentBound` (two `size_t` parameters) has the swappable parameters category, which the file already has 18 times; the parameters are documented in the header |

The changed headers (`MapPainter.h`, `LabelLayouter.h`, `StyleConfig.h`, `LabelLayouterHelper.h`) and `MapPainter.cpp` were not run through clang-tidy separately: they carry the pre-existing findings of the painter files (documented in `TODO.md` for the backends), and the new code in them follows the categories the files already have.

## 6. Files changed

| file | change |
|---|---|
| `libosmscout-map/include/osmscoutmap/StyleConfig.h`, `src/osmscoutmap/StyleConfig.cpp` | the `VisibilityBounds` record, its per-level derivation and the accessor |
| `libosmscout-map/include/osmscoutmap/LabelLayouterHelper.h`, `src/osmscoutmap/LabelLayouterHelper.cpp` | `CountLabelWords`, `GetLabelExtentBound`, `GetMaxLabelPaddingPixel`, `GetLabelLayoutMarginPixel` |
| `libosmscout-map/include/osmscoutmap/LabelLayouter.h` | the early label decision, its gating on the frame's viewport, the debug assertion, and the fix of the uninstantiable `RegisterLabel` overload |
| `libosmscout-map/include/osmscoutmap/MapPainter.h`, `src/osmscoutmap/MapPainter.cpp` | the visibility bounds per database, `CanWayBeVisible`, `IsPointVisible`, the way, shield and point object decisions |
| `Tests/src/StyleConfigVisibilityBoundsTest.cpp`, `Tests/src/MapPainterWayCullingTest.cpp`, `Tests/src/MapPainterPointObjectCullingTest.cpp`, `Tests/src/MapPainterLabelCullingTest.cpp` | new tests |
| `Tests/CMakeLists.txt`, `Tests/meson.build` | registration of the new tests |
| `TODO.md` | the findings of this work (see there) |
