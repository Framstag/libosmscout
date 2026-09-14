# Verification: map-painter-label-reuse

Evidence collected while applying the tasks in `tasks.md`.

## Environment

- Build: `build/` (CMake, Ninja, `CMAKE_BUILD_TYPE=Release`), map backends Cairo, Qt, Skia,
  SVG, OpenGL and AGG enabled, `libtcmalloc.so.4` linked.
- Database: `maps/Dortmund` (508 types, imported 2025-08-17 from `maps/Dortmund.osm.pbf`).
- Stylesheet: `stylesheets/standard.oss`, icons `libosmscout/data/icons`.
- Driver: `--driver cairo` for all label measurements, because the `noop` driver skips label
  registration and text measurement entirely (`MapPainterNoOp.cpp:35`, `:50`).
- The allocation counter of `Tests/src/PerformanceTest.cpp` counts every `operator new`
  (`:279-312`), including allocations made inside Pango, GLib and Cairo.

## 1.1 Baseline before the change

Fixed view, single tile at zoom 15 covering the Dortmund city centre (the view the two
preceding painter changes used, so the numbers are comparable):

```
./build/Tests/PerformanceTest \
  --driver cairo --start-zoom 15 --end-zoom 15 \
  --draw-repeat 5 --load-repeat 1 --icons libosmscout/data/icons \
  maps/Dortmund stylesheets/standard.oss 51.514 7.463 51.510 7.470
```

Loaded data of that view: 164 nodes, 6308 ways, 17629 areas per frame.
Frame average: 23.37 ms, 4637 allocations per frame.

| step | name | allocs/frame | alloc share | ms/frame | time share |
|------|------|--------------|-------------|----------|------------|
| #18 | PrepareNodeLabels | 651 | 14 % | 0.44 | 2 % |
| #22 | DrawLabels | 2006 | 43 % | 1.41 | 6 % |
| #3 | CalculateWayShields | 860 | 19 % | 2.33 | 10 % |
| #14 | DrawWayContourLabels | 777 | 17 % | 0.48 | 2 % |
| #15 | PrepareAreaLabels | 150 | 3 % | 0.50 | 2 % |
| | **label steps** | **4444** | **96 %** | **5.16** | **22 %** |
| #4 | ProcessAreas | 32 | 1 % | 3.03 | 13 % |
| #11 | DrawAreas | 5 | 0 % | 7.15 | 31 % |

Band view at zoom 16, 16 tiles, `--draw-repeat 3`, coordinates `51.45 7.35 51.46 7.37`
(allocation counts are per tile-frame, i.e. divided by 16 tiles x 3 frames; the frame average
is 7.82 ms, 10518 allocations, so the per-tile numbers add up to the per-frame total):

| step | name | allocs/tile-frame | alloc share | ms/tile-frame | time share |
|------|------|-------------------|-------------|---------------|------------|
| #18 | PrepareNodeLabels | 3595 | 34 % | 1.05 | 13 % |
| #22 | DrawLabels | 5295 | 50 % | 1.50 | 19 % |
| #3 | CalculateWayShields | 615 | 6 % | 0.55 | 7 % |
| #14 | DrawWayContourLabels | 429 | 4 % | 0.19 | 2 % |
| #15 | PrepareAreaLabels | 410 | 4 % | 0.68 | 9 % |
| | **label steps** | **10344** | **98 %** | **3.97** | **50 %** |
| #4 | ProcessAreas | 5 | 0 % | 0.44 | 6 % |
| #11 | DrawAreas | 4 | 0 % | 1.07 | 14 % |

Every remaining step of the frame accounts for the residual 1-2 % of allocations, i.e. the
label stage is the frame's allocation budget at both zoom levels, and at zoom 16 it is also
half of its time.

Note on comparability: the zoom 15 fixed view measures one tile drawn five times, so its
per-frame label count is a single tile's worth, while the zoom 16 view measures 16 tiles drawn
three times. The two views therefore expose the per-label and the per-tile scaling of the same
steps.

## 1.2 Where the label allocations sit

Measured with the temporary probe (`OSMSCOUT_LABEL_PROBE=<file>`, reverted at the end of the
change; see section 8.6). The probe wraps each label-stage entry point in a scope that
records the call count and the allocations performed inside it, so the buckets are inclusive
unless marked otherwise. The count comes from the allocation counter of the test binary
(`Tests/src/PerformanceTest.cpp:349`), which counts every `operator new`.

**Important limitation**: Pango, GLib and Cairo allocate their internal objects with their own
allocators (`g_slice`, plain `malloc`), which the replaced `operator new` does not see. The
counted allocations are therefore libosmscout's own C++ allocations - containers, `shared_ptr`
control blocks, `std::string` copies - and a text layout shows up as 2 allocations (the
`make_shared` block of the label and its text) while its shaping cost is invisible here and
only shows up in the step's time.

### Zoom 15 fixed view, per frame (steady state, frame 4 of 5)

| bucket | kind | calls | allocs | allocs/call |
|--------|------|-------|--------|-------------|
| `layoutPointLabels` | inclusive | 253 | 789 | 3.1 |
| `registerPointWayLabel` | inclusive | 104 | 704 | 6.8 |
| `registerLabel` | inclusive | 334 | 1003 | 3.0 |
| `registerContourLabel` | inclusive | 34 | 631 | 18.6 |
| `processLabel` | inclusive | 337 | 665 | 2.0 |
| `measurement` | leaf | 192 | 390 | 2.0 |
| `glyphDerivation` | leaf | 34 | 502 | 14.8 |
| `processLabelInstance` | inclusive | 334 | 1930 | 5.8 |
| `processLabelContourLabel` | inclusive | 11 | 51 | 4.6 |
| `layoutJob` | inclusive | 1 | 1988 | - |
| `layouterDrawLabels` | inclusive | 1 | 24 | - |
| `painterDrawLabels` | inclusive | 1 | 2012 | - |
| `maskConstructor` | leaf | 375 | 375 | 1.0 |
| `canvasConstructor` | leaf | 3 | 3 | 1.0 |
| `pathGeometry` | inclusive | 34 | 137 | 4.0 |
| `shieldGrid` | inclusive | 375 | 150 | 0.4 |

### Zoom 16 band view, per tile-frame (16 tiles x 3 frames)

| bucket | kind | calls | allocs | allocs/call |
|--------|------|-------|--------|-------------|
| `layoutPointLabels` | inclusive | 787.1 | 3997.0 | 5.1 |
| `registerPointWayLabel` | inclusive | 57.8 | 500.7 | 8.7 |
| `registerLabel` | inclusive | 842.4 | 2654.2 | 3.2 |
| `registerContourLabel` | inclusive | 12.0 | 336.3 | 28.0 |
| `processLabel` | inclusive | 979.0 | 1763.2 | 1.8 |
| `measurement` | leaf | 321.4 | 698.7 | 2.2 |
| `glyphDerivation` | leaf | 12.0 | 256.1 | 21.3 |
| `processLabelInstance` | inclusive | 842.4 | 5155.9 | 6.1 |
| `processLabelContourLabel` | inclusive | 8.0 | 119.8 | 15.0 |
| `layoutJob` | inclusive | 1.0 | 5282.6 | - |
| `layouterDrawLabels` | inclusive | 1.0 | 27.0 | - |
| `painterDrawLabels` | inclusive | 1.0 | 5149.6 | - |
| `maskConstructor` | leaf | 1083.4 | 1083.4 | 1.0 |
| `canvasConstructor` | leaf | 3.0 | 3.0 | 1.0 |
| `pathGeometry` | inclusive | 12.0 | 75.2 | 6.3 |
| `shieldGrid` | inclusive | 140.1 | 110.7 | 0.8 |

### Attribution of the per-step allocation counts (zoom 16, per tile-frame)

The inclusive buckets partition the label steps, and the partition adds up to the step counts
the test reports:

| step | probe buckets | probe allocs | step allocs |
|------|---------------|--------------|-------------|
| #18 + #15 | `layoutPointLabels` | 3997.0 | 3595 + 410 = 4005 |
| #3 | `shieldGrid` + `registerPointWayLabel` | 611.4 | 615 |
| #14 | `pathGeometry` + `registerContourLabel` | 411.5 | 429 |
| #22 | `painterDrawLabels` | 5149.6 | 5295 |
| | **label steps** | **10170** | **10344** |
| | frame total | | 10537 |

`painterDrawLabels` trails the reported step by about 3 % because the step also covers the
backend work around the layouter call. `layoutPointLabels` is the per-object preparation of
both `PrepareNodeLabels` and `PrepareAreaLabels`, which is why it maps onto their sum.

### What the buckets are made of

- `measurement` is 2.2 allocations per measurement at zoom 16 (698.7 / 321.4): the
  `make_shared` block of the label object and the copy of the label text. It is the target of
  the measurement reuse, and it is the *complete* counted cost of measuring a label - the
  shaping cost inside Pango is not counted here but is visible in the step times (section 1.1).
- `processLabelInstance` is the largest single bucket (5155.9, i.e. 49 % of the frame total):
  1083.4 of it are the bitmask of each `ScreenRectMask`, 842.4 the outer `masks` vector, 842.4
  the `canvases` vector, and the rest the visible-element list, the instance copy that is
  pushed into the ordered store, and the growth of the element vector. All of it except the
  instance copy is scratch storage.
- `glyphDerivation` is 21.3 allocations per path label at zoom 16, i.e. one `shared_ptr`
  control block per glyph plus the font reference of each run (the
  `pango_glyph_string_new` allocations themselves are invisible to the counter).
- `pathGeometry` is the `LabelPath` of a path label with its two growing vectors.
- `shieldGrid` is the `std::set` of grid points that `GetGridPoints` builds per shielded way;
  `registerPointWayLabel` is only entered by the 57.8 of 140.1 calls per tile-frame whose grid
  set is not empty, so the 8.7 allocations per entered call include the label data vector and
  the label registration of each grid point.
- `layoutPointLabels` owns about 1.9 allocations per call (1517 of its 3997 allocations are
  not nested registrations at zoom 16): the per-object label list and the copies of the label
  data.
- `canvasConstructor` is small in count but large in bytes: three viewport-sized bitmaps per
  frame (at 256x256 tiles plus overlap, about 100 KB each).
- `layouterDrawLabels` allocates the two postponed-element pointer vectors of a frame.
- `painterDrawLabels` (5149.6, the #22 step) was assumed to be the drawing of the resolved labels.
  The probe shows it *contains* `layoutJob` (5282.6 on average), i.e. the step's allocations are
  the layouter's overlap resolution, and the drawing itself allocates almost nothing (at zoom 15:
  2012 - 1988 = 24 allocations per frame). The symbol raster cache of `TODO.md` is therefore a
  frame-time item, not an allocation item. Note the one-frame offset of this outermost bucket
  (see the probe notes below) when comparing it to `layoutJob`.

### Expected effect of this change

On a repeated frame of the same view, the requirements target `measurement` (698.7),
`glyphDerivation` (256.1), the scratch part of `processLabelInstance` (~3600 of 5155.9),
`pathGeometry` (75.2), `shieldGrid` (110.7), `canvasConstructor` (3) and `layouterDrawLabels`
(27), plus the per-object label list (~787 of the 1517 scaffolding allocations of
`layoutPointLabels`). That is about 5560 of the 10537 allocations per tile-frame at zoom 16,
i.e. roughly half of the frame's allocations. Section 8.5 records the measured result.

## 1.3 Reference label placement of the fixed view

`baseline-labels.txt` (this directory) holds one line per label that takes part in the fixed
view's frame 4 (the same view as 1.1), in draw order, with position, size, priority, kind and
text:

```
0 symbol x=12.885 y=32.054 w=5.669 h=7.559 priority=18446744073709551615 label=christian_church_cross
...
27 text x=248.500 y=182.299 w=15.000 h=6.000 priority=22 label=B 54
...
33 overlay x=85.783 y=36.797 w=79.000 h=28.000 priority=11 label=Mitte
34 contour x=0.000 y=0.000 w=0.000 h=0.000 priority=26 label=
35 contour x=0.000 y=0.000 w=0.000 h=0.000 priority=26 label=
```

The dump has 36 lines: 27 symbols, 6 text labels, 1 overlay label and 2 contour labels. The
order is the draw order - symbols and icons first, then the postponed text labels, then the
overlay labels, then the contour labels - so the file captures the label set, the placement
and the draw order of the frame. The `symbol` and `text` prefixes are the element kinds the
layouter draws; the contour lines carry no text because `ContourLabel` keeps its text only in
debug builds (`LabelLayouter.h:211-215`).

Command:

```
OSMSCOUT_LABEL_PROBE=/tmp/label-probe.txt ./build/Tests/PerformanceTest \
  --driver cairo --start-zoom 15 --end-zoom 15 --draw-repeat 5 --load-repeat 1 \
  --icons libosmscout/data/icons \
  maps/Dortmund stylesheets/standard.oss 51.514 7.463 51.510 7.470
# PLACE lines of frame 4, with the frame and index columns stripped
```

### Probe implementation notes

- The probe shares one instance across the map libraries and the test binary
  (`GetLabelProbe()` is declared in `include/osmscoutmap/LabelProbe.h` and defined in
  `src/osmscoutmap/LabelLayouterHelper.cpp`). An inline definition was duplicated per shared
  library because the libraries build with hidden visibility, which silently dropped the
  counters of `libosmscout-map` from the frame summary.
- The allocation counter has to be handed to the probe by the test binary
  (`osmscout::GetLabelProbe().SetAllocationCounter(&GetAllocationCount)`), because the counter
  lives in the test's replacement of the global `operator new`.
- `painterDrawLabels` is the outermost scope of a frame and ends after `Reset()` has written
  the frame summary, so its value appears in the summary of the *next* frame. All other
  buckets are unaffected.
- The probe run is slower than the uninstrumented run (76 ms/frame instead of 23 ms/frame on
the fixed view at the time of writing), so probe runs are used for counts only; the timings of
section 1.1 come from uninstrumented runs.

## 2. Reused label measurements and glyphs

Change: `LabelLayouter` remembers a label measurement per measurement key (text, font size,
proposed width, wrapping, contour flag) up to `defaultMeasurementCount` (4096) entries, with
FIFO eviction that drops the glyph entry of an evicted measurement with it. A backend sets the
measurement environment it measures in once per frame (`SetMeasurementEnvironment`), and a
change of that environment drops both stores. The Cairo backend derives its environment from
the font name, the font size factor, the DPI, the magnification, the drawing target's font
options and its device scale (`MapPainterCairo::GetMeasurementEnvironment`); Qt appends the
DPI and the device pixel ratio of the painter device; Skia, SVG, AGG, GDI and DirectX use the
shared `BuildMeasurementEnvironment` prefix (font name, font size factor, DPI, magnification).

Evidence (fixed view, zoom 15, cairo, per frame, probe buckets of section 1.2):

| bucket | before | after |
|--------|--------|-------|
| `measurement` (allocs) | 390 | 6 |
| `measurement` (calls) | 192 | 192 |
| `measurementMiss` (calls) | - | 0 |
| `glyphDerivation` (allocs) | 502 | 0 |
| `registerContourLabel` (allocs) | 631 | 61 |
| `processLabel` (allocs) | 665 | 349 |
| `registerLabel` (allocs) | 1003 | 687 |
| `layoutPointLabels` (allocs) | 789 | 773 |
| frame allocations | 4647 | 3880 |
| frame allocations (3-frame run) | 4637 | 3921 |

- The 192 measurement calls of a repeated frame perform no measurement at all (`measurementMiss`
  is 0 in every frame after the first), and the 6 allocations that remain are the string buffers
  of the measurement key for the five distinct labels whose text exceeds the small-string
  buffer (16, 17, 18, 19 and 35 characters), one of them registered twice per frame.
- The first frame performs 25 measurements for 192 measurement calls: the measurement key
  collapses the repeated registrations of the same label (shields placed at several grid
  points, route and border labels of one object) within a frame as well as across frames.
- `glyphDerivation` is 0 in the steady state, i.e. a path label's glyphs are derived once.
- **The rendered label placement is unchanged**: the placement dump of the fixed view's frame 4
  is byte-identical to `baseline-labels.txt` (36 lines, `diff` reports no difference), so label
  set, position, size, priority, kind and draw order are unchanged by the measurement reuse.
- `MapPainterShieldTest`, `MapPainterRouteTest`, `MapPainterFrameBuffersTest`, `ScreenMaskTest`,
  `LabelPathTest` and the `TextMetrics*` tests pass unchanged.
- Backends whose code cannot be built in this environment (AGG, GDI and DirectX are not enabled
  in the local CMake build, the iOS backend builds only on macOS) were changed by inspection:
  each has the label layouter as a member and receives the projection and the parameters in its
  `DrawMap`, which is where the environment is set. Their compilation is verified by CI
  (`build_and_test_on_msys.yml`, `build_and_test_on_vs2025.yml`, `build_and_test_on_ios.yml`),
  not by this run. `libosmscout-map-iosx` (`MapPainterIOS.mm`) was deliberately left unchanged
  and keeps an empty environment; see section 6.1.

## 3. Reused frame state and label stage scratch

Change: `LayoutJob` became reusable state of the layouter (`layoutJob`), `ScreenMask` and
`ScreenRectMask` gained a `Reset` that reuses their bitmask, the per-instance masks/canvases
moved into the job, the resolved instance is built directly in the output store (no
visible-element list, no instance copy), `MapPainter::LayoutPointLabels` reuses a painter member
for its label list, `LabelPath` gained `Clear` and the painter reuses one path for path labels,
and `GetGridPoints` fills a reused sorted unique buffer instead of building a `std::set` per
shielded way.

Evidence (fixed view, zoom 15, cairo, per frame, probe buckets of section 1.2):

| bucket | before | after |
|--------|--------|-------|
| `processLabelInstance` | 1930 | 314 |
| `maskConstructor` | 375 | 0 |
| `canvasConstructor` | 3 | 0 |
| `pathGeometry` | 137 | 0 |
| `shieldGrid` | 150 | 0 |
| `layoutPointLabels` | 773 | 586 |
| `layoutJob` | 1983 | 325 |
| `painterDrawLabels` | 2007 | 349 |
| **frame allocations** | **3924** | **1750** |

Zoom 16 band view, per tile-frame (48 tile-frames), all buckets averaged over the run:

| bucket | before | after |
|--------|--------|-------|
| `processLabelInstance` | 5155.9 | 974.7 |
| `maskConstructor` | 1083.4 | 0.0 |
| `canvasConstructor` | 3.0 | 0.0 |
| `pathGeometry` | 75.2 | 0.2 |
| `shieldGrid` | 110.7 | 0.1 |
| `measurement` | 698.7 | 169.7 |
| `glyphDerivation` | 256.1 | 22.5 |
| `layoutPointLabels` | 3997.0 | 2838.4 |
| `registerPointWayLabel` | 500.7 | 279.8 |
| `registerContourLabel` | 336.3 | 83.7 |
| `layoutJob` | 5282.6 | 996.3 |
| `painterDrawLabels` | 5149.6 | 990.9 |
| **allocation count of the view** | **10537** | **4505** |

The zoom 16 view has one painter per tile, so the first frame of every tile measures its labels
for the first time: the remaining `measurement` (169.7) and `glyphDerivation` (22.5) allocations
average over those first frames and over the measurement-key strings of the remaining frames
(`measurementMiss` is 0 in every frame after a tile's first one). The remaining
`layoutPointLabels` (2838.4) is dominated by the copies of the label data into the label
instances and the string copies of the label text, i.e. by data the frame's output owns.

- **A defect was caught by the placement dump**: making `LayoutJob` a persistent member
  invalidated the assumption that the stores swapped into the output vectors are empty, so
  `ProcessLabels` appended the resolved labels to the previous frame's registered labels and the
  dump showed every label twice (98 instead of 36 entries). `LayoutJob::PrepareFrame` now clears
  the output stores after the swap, and the dump is byte-identical again. This is why the dump of
  section 1.3 is part of the change.
- `ScreenMask::Reset`/`ScreenRectMask::Reset` keep their size when the viewport and the mask
  width are unchanged, so a repeated frame does not allocate (`canvasConstructor` and
  `maskConstructor` are 0), while a changed layout viewport resizes the bitmask.
- The remaining `layouterDrawLabels` (24 allocations per frame) is the two postponed-element
  pointer vectors of the layouter's draw step; they are the last scratch vectors of the label
  stage that this change does not reuse.

## 4. Whole-suite regression check after groups 2-5

```
cd build && QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR=<src>/Tests TESTS_TMP_DIR=<build>/Tests \
  ctest -j 4 --output-on-failure
-> 100% tests passed out of 117
```

Includes `MapPainterShieldTest`, `MapPainterShieldQtTest`, `MapPainterRouteTest`,
`MapPainterFrameBuffersTest`, `LabelPathTest`, `ScreenMaskTest`, the `TextMetrics*` tests, the
SVG/Cairo/Qt/Skia symbol and drawing tests and all stylesheet checks. No golden output changed.

## 5. Unit tests

`Tests/src/MapPainterLabelReuseTest.cpp` (new, registered in `Tests/CMakeLists.txt` and
`Tests/meson.build`), 9 test cases, 179 assertions. It drives a `LabelLayouter` with a counting
fake text layouter, so it needs no database, no stylesheet and no rendering backend; the glyph
hook of the fake label type is an explicit specialization that counts derivations.

| requirement (spec) | test |
|--------------------|------|
| A repeated frame reuses all measurements of unchanged labels | `A repeated frame measures no label again` |
| More labels do not increase the number of measurements of a repeated frame | `A repeated frame measures no label again` (third frame registers 20 labels) |
| A changed measurement input is measured again | `A changed measurement input is measured again` |
| A changed drawing parameter is measured again | `A changed measurement environment is measured again` |
| Reuse does not depend on a label having been drawn | `Reuse does not depend on the label having been drawn` |
| Reused and fresh measurements agree | `Reuse does not depend on the label having been drawn` (second half) |
| Glyphs of a label that takes part in two frames are derived once | `Glyphs of a label that takes part in two frames are derived once` |
| Reused glyph data equals freshly derived glyph data | same test, second half |
| Path labels reuse glyph data as well | same test (path labels are the glyph-carrying kind) |
| An environment change drops stale glyphs | same test, after `SetMeasurementEnvironment` |
| Label set, placement and draw order are unchanged | `The label set, placement and order are unchanged` |
| The label stage's allocations do not grow with the frame number | `The label stage allocates a constant amount per frame` |
| Labels that do not take part in the frame do not add allocations | `Labels that do not take part in the frame do not allocate` |
| The cache bound is honoured | `The measurement cache stays within its bound` |

Additional cases in the existing test files:

- `Tests/src/ScreenMaskTest.cpp`: `ScreenMask reset clears the marks of a frame` (a frame's
  marks do not leak into the next frame, a taller viewport keeps them away, a smaller viewport
  ignores masks below it) and `ScreenRectMask reset rebuilds the mask` (a reset mask reports the
  same cells as a freshly built one, a rectangle beyond the screen width is empty, a wider screen
  needs further cells).
- `Tests/src/LabelPathTest.cpp`: `LabelPath clear drops the points and keeps the path usable`
  (length back to 0, then a rebuilt path with correct length, position and angle).

### Sensitivity of the new tests

Checked by building the test against a copy of the library in which the reuse is neutralized
(the measurement cache and the glyph table are never consulted, the mask/canvas stores are
re-created per use, the bitmasks are re-created per use; the patch was reverted and the files
restored from a backup afterwards). 7 of the 9 cases fail, with:

```
A repeated frame measures no label again                 measurementCount==measurementsOfFirstFrame  ->  12 == 6
A changed measurement environment is measured again      measurementCount==1                        ->  2 == 1
Reuse does not depend on the label having been drawn     measurementCount==2                        ->  4 == 2
Glyphs of a label that takes part in two frames ...       measurementCount==1                        ->  2 == 1
The label stage allocates a constant amount per frame    allocationsOfThirdFrame<40                 ->  92 < 40
Labels that do not take part in the frame do not allocate heavyAllocations<=lightAllocations+8    ->  1019 <= 27
The measurement cache stays within its bound             measurementCount==3                        ->  6 == 3
```

The two cases that pass without reuse are the two regression guards ("A changed measurement
input is measured again" asserts an increase, "The label set, placement and order are
unchanged" asserts equality), which is what they are for.

## 6. Backend measurement environment review

Every backend that instantiates a `LabelLayouter` was reviewed for measurement inputs that are
neither part of the measurement key (text, font size, proposed width, wrapping, contour flag)
nor of the environment the backend sets:

| backend | measurement inputs | environment | verdict |
|---------|--------------------|-------------|---------|
| Cairo (`MapPainterCairo`) | font name, font size factor, DPI, magnification (`GetFont`), target font options, target device scale (Pango layout), constant hinting on the non-Pango path | all six | complete |
| Qt (`MapPainterQt`) | font name, font size factor, DPI, magnification (`GetFont`), `QFontMetrics` of the painter device | prefix + device DPI + device pixel ratio | complete |
| Skia (`MapPainterSkia`) | typeface of the font name, font size factor, DPI, magnification; `SkFontMetrics` does not depend on the canvas | prefix | complete |
| SVG (`MapPainterSVG`) | font name, font size factor, DPI, magnification; the Pango font map resolution is fixed when the painter is created | prefix | complete |
| AGG (`MapPainterAgg`) | font file of the font name, font size factor, DPI, magnification, constant hinting | prefix | complete |
| GDI (`MapPainterGDI`) | font name, font size factor, DPI, magnification | prefix | complete, with the note below |
| DirectX (`MapPainterDirectX`) | font name, font size factor, DPI, magnification; `IDWriteTextFormat` metrics | prefix | complete |
| iOS (`MapPainterIOS`) | font name, font size factor, DPI, magnification (CoreText) | prefix | complete, not built locally (see below) |

- **Known gap (GDI)**: `Gdiplus` text metrics also depend on the DPI of the device context the
  painter draws to. The environment covers the projection's DPI but not a change of the HDC to
  one with another DPI. Windows-only, requires an application that switches device contexts with
  different resolutions while keeping one painter and one projection; recorded as a follow-up.
- **Not built locally**: AGG, GDI, DirectX and the iOS backend are not part of the local CMake
  configuration, so their changes are verified by review (each has the label layouter as a
  member and receives projection and parameters in its `DrawMap`) and by CI
  (`build_and_test_on_msys.yml`, `build_and_test_on_vs2025.yml`, `build_and_test_on_ios.yml`).
- `MapPainterNoOp` registers no labels at all (`MapPainterNoOp.cpp:50`, `:62`), so it has no
  measurement environment to set.

## 8. Verification

### 8.1 CMake build

`cmake --build build` (Release, Ninja): exit 0, no error. No warning in any changed file; the
34 warnings of the full build are in untouched files (`libosmscout-client-qt/MapWidget.cpp`,
`Voice*.cpp`, `MapDownloader.cpp` deprecated Qt APIs, the pre-existing
`MapPainterOpenGL.cpp:464` warning of the previous change, and vendored `nanosvg.h`).

### 8.2 Meson build

The build directory had to be regenerated after `Tests/meson.build` changed
(`meson setup --reconfigure build-meson`); afterwards:

```
meson compile -C build-meson MapPainterLabelReuseTest ScreenMaskTest LabelPathTest \
  MapPainterAreaPreparationTest   -> exit 0, no warning, no error
meson test -C build-meson "Check MapPainterLabelReuse compilation" \
  "Check LabelPath code" "Check ScreenMask functionality"  -> Ok: 3, Fail: 0
```

The first Meson build of `Tests/src/MapPainterLabelReuseTest.cpp` reported 13
`-Wmismatched-new-delete` warnings: the file replaces the global `operator new`/`operator delete`
with `malloc`/`free` *and* instantiates the label layouter template, which makes GCC report that
pairing as a mismatch. The diagnostic is disabled around the replaced operators in the test file,
the way the other counter tests avoid it by instantiating the layouter in a library.

### 8.3 Test suite

```
cd build && QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR=<src>/Tests TESTS_TMP_DIR=<build>/Tests \
  ctest -j 4
-> 100% tests passed out of 118 (43.1 s)
```

118 instead of 117 because `MapPainterLabelReuseTest` is new. Includes the label, shield, route,
text metric, mask, path, SVG/Cairo/Qt/Skia symbol and stylesheet tests and all
`PerformanceTest-*` invocations.

### 8.4 Linters

Uncrustify 0.83.0_f (`script/format-check.sh` requires that version). The new test file was
formatted with the project configuration and has **0 drift**. For the changed existing files the
drift count (lines that differ between `uncrustify`'s output and the file) was compared against
the drift of the same file at `HEAD`:

| file | HEAD | working tree |
|------|------|--------------|
| `Tests/src/MapPainterLabelReuseTest.cpp` (new) | - | 0 |
| `Tests/src/ScreenMaskTest.cpp` | 75 | 75 |
| `libosmscout-map/include/osmscoutmap/LabelLayouter.h` | 316 | 295 |
| `libosmscout-map/include/osmscoutmap/LabelLayouterHelper.h` | 49 | 49 |
| `libosmscout-map/include/osmscoutmap/LabelPath.h` | 8 | 7 |
| `libosmscout-map/include/osmscoutmap/MapPainter.h` | 108 | 108 |
| `libosmscout-map/src/osmscoutmap/LabelLayouterHelper.cpp` | 5 | 5 |
| `libosmscout-map/src/osmscoutmap/LabelPath.cpp` | 60 | 60 |
| `libosmscout-map/src/osmscoutmap/MapPainter.cpp` | 521 | 524 |
| `libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp` | 359 | 367 |
| `libosmscout-map-qt/src/osmscoutmapqt/MapPainterQt.cpp` | 170 | 172 |
| `libosmscout-map-gdi/src/osmscoutmapgdi/MapPainterGDI.cpp` | 338 | 344 |
| `libosmscout-map-iosx/src/osmscout/MapPainterIOS.mm` | 1770 | 1787 |
| `libosmscout-map-{agg,skia,svg,directx}/...` | unchanged | unchanged |

- The added lines themselves are uncrustify-clean wherever the tool's suggestion was sane: the
  constructor, the new members, the new methods and the backend environment calls were aligned
  to the tool's output.
- The remaining increases (+1 to +17 lines) are drift that uncrustify produces *around* the added
  statements: the tool aligns whole declaration blocks, so an added statement re-aligns its
  neighbouring pre-existing lines (visible in `MapPainter.cpp`'s `double xCoord=...` and in the
  `private:` block of `LabelLayouter.h`, whose count nevertheless went *down*).
- Two suggestions were deliberately not adopted because they are worse than the current code:
  uncrustify indents the body of a lambda to the column of its capture list (avoids: the new test
  case uses a named helper function instead of a lambda, which removed those hunks), and
  `.uncrustify` wants right-attached pointers while `guidelines/CodeStyles.md` documents
  left-attached (the previous change recorded the same conflict).
- The `iosx` increase (+17) is in a file that is not built here; its four added lines follow the
  pattern of the other backends and the file's 4-space indentation.

clang-tidy (`.clang-tidy`, `clang-tidy -p build`):

| file | findings in the file | dominant checks |
|------|---------------------|-----------------|
| `Tests/src/MapPainterLabelReuseTest.cpp` (new) | 431 | `avoid-magic-numbers` 187, `pro-bounds-avoid-unchecked-container-access` 59, `no-malloc`/`owning-memory` 8 each |
| `Tests/src/MapPainterAreaPreparationTest.cpp` (reference, existing) | 155 | `avoid-magic-numbers` 68, `pro-bounds-avoid-unchecked-container-access` 10, `modernize-use-emplace` 9 |
| `libosmscout-map/src/osmscoutmap/LabelLayouterHelper.cpp` | 34 | `pro-bounds-avoid-unchecked-container-access` 11, `modernize-avoid-c-style-cast` 7 |

The new code introduces no check category that the files did not already carry: the allocation
counter contributes the same `no-malloc`/`owning-memory` findings as in the reference test, the
test literals contribute `avoid-magic-numbers` (the reference test carries 68 of them), and the
container accesses of the tests contribute `pro-bounds-avoid-unchecked-container-access` (the
check the previous change recorded as pre-existing in its changed region). One finding is worth
naming: `bugprone-easily-swappable-parameters` on the new
`BuildMeasurementEnvironment(fontName, fontSize, dpi, magnification)` (two adjacent `double`
parameters). Passing a small struct instead of four parameters would remove it; the call sites are
one per backend, so this is a free-standing follow-up rather than part of this change.

## 9. Final measurement

Baseline (section 1.1) against the change, both uninstrumented, same views and commands:

| metric | before | after | change |
|--------|--------|-------|--------|
| zoom 15 fixed view, allocations per frame | 4637 | 1740 | -62 % |
| zoom 16 band view, allocations per tile-frame | 10537 | 4486 | -57 % |
| zoom 15, step 18 `PrepareNodeLabels` allocations | 651 | 173 | -73 % |
| zoom 15, step 15 `PrepareAreaLabels` allocations | 150 | 41 | -73 % |
| zoom 15, step 3 `CalculateWayShields` allocations | 860 | 286 | -67 % |
| zoom 15, step 14 `DrawWayContourLabels` allocations | 777 | 51 | -93 % |
| zoom 15, step 22 `DrawLabels` allocations | 2006 | 361 | -82 % |
| zoom 15 frame allocations (3-frame run) | 4637 | 1785 | -61 % |

Per-step numbers are the averages the test prints (`allocs / frames`); the zoom 16 view is
16 tiles x 3 frames so its numbers are per tile-frame.

The remaining label-stage allocations are the data the frame's output owns: the copies of the
label data into the resolved label instances, the string buffers of the measurement key of long
labels, the glyph copies into `ContourLabel::glyphs`, and the two postponed-element pointer
vectors of the layouter's draw step (24 per frame at zoom 15).

### Label placement and render output

- The label placement dump of the fixed view's frame 4 is byte-identical to
  `baseline-labels.txt` (36 lines: 27 symbols, 6 text labels, 1 overlay, 2 contour labels) - the
  same comparison was used at every implementation step, including the step at which it exposed
  the duplicated labels of section 3.
- `MapPainterShieldTest`, `MapPainterShieldQtTest`, `MapPainterRouteTest`,
  `MapPainterFrameBuffersTest`, `LaneEvaluationCompare` and the SVG/Cairo/Qt/Skia drawing and
  symbol tests pass with unchanged golden output.

### Defect found and fixed while applying the change

Making `LayoutJob` a persistent member broke the frame twice over, and both parts were caught by
these verifications rather than by inspection:

1. **Duplicated labels** (caught by the placement dump): the stores that `LayoutJob` swaps into the
   output vectors are only empty when the job is created per frame. With a persistent job they
   hold the previous frame's registered labels, which `ProcessLabels` then appended to
   (98 instead of 36 placement lines). Fixed by `LayoutJob::PrepareFrame`, which clears the output
   stores after the swap. Section 3 records the numbers.
2. **Self-deadlock of the Cairo painter** (caught by the full `ctest` run and reproduced standalone
   with `PerformanceTest-cairo-public-transport.oss`): the edit that removed the temporary probe
   from `MapPainterCairo::DrawLabels` inserted a `std::lock_guard<std::mutex> guard(mutex)` there
   by mistake. `DrawMap` already holds that non-recursive mutex while it calls the step methods, so
   every Cairo frame deadlocked in step 22 with all threads waiting on the mutex; the `noop`
   driver (which registers no labels) was unaffected. Fixed by removing the lock again; the whole
   suite passes afterwards. This is the reason the full suite is part of the task list, and it is
   recorded here rather than in `TODO.md` because it was introduced and removed within the change.


## 10. Chronological note

The whole-suite check of section 4 is the one that was run directly after the implementation
groups 2-5; the runs recorded in section 8 are the final ones of the change.

