# Verification: map-painter-area-preparation

Evidence collected while applying the tasks in `tasks.md`.

## Environment

- Build: `build/` (CMake, Ninja, `CMAKE_BUILD_TYPE=Release`), map backends Cairo, Qt, Skia,
  SVG, OpenGL enabled.
- Database: `maps/Dortmund` (508 types, imported 2025-08-17 from `maps/Dortmund.osm.pbf`).
- Stylesheet: `stylesheets/standard.oss`, icons `libosmscout/data/icons`.
- Driver note: `--driver noop` runs the whole painter pipeline with no-op backend calls and
  `MapPainterNoOp` skips label registration and text measurement entirely
  (`MapPainterNoOp.cpp:35`, `:50`), so noop numbers isolate painter-side preparation.
  Wall time is also reported for `cairo`.

## 1.1 Baseline before the change

Fixed view, single tile at zoom 15 covering the Dortmund city centre (the same view the
`map-painter-frame-containers` baseline used, so the numbers are comparable):

```
./build/Tests/PerformanceTest \
  --driver noop|cairo --start-zoom 15 --end-zoom 15 \
  --draw-repeat 5 --load-repeat 1 --icons libosmscout/data/icons \
  maps/Dortmund stylesheets/standard.oss 51.514 7.463 51.510 7.470
```

Loaded data of that view: 164 nodes, 6308 ways, 17629 areas (per frame).
Prepared data of that view: **726 prepared area entries per frame**.

| metric | noop | cairo |
|--------|------|-------|
| allocations per frame (all steps) | 53901 | 57544 |
| allocations per frame, step 4 `ProcessAreas` | 52939 (98 %) | 52939 (92 %) |
| step 4 draw time, ms/frame | 9.03 | 11.78 |
| step 4 share of frame time | 39 % | 22 % |
| step 2 `CalculatePaths`, ms/frame | 3.90 | 4.30 |
| step 11 `DrawAreas`, ms/frame | - (noop) | 13.49 |
| step 22 `DrawLabels`, ms/frame | 0.45 | 3.15 |
| DB load, ms | 22.88 | 34.06 |
| frame total, avg ms | 23.29 | 53.17 |

Band baselines, small tile box `51.45 7.35 51.46 7.37`, `--draw-repeat 3`, noop
(allocation counts are backend-independent: the cairo runs of the same views reported the
identical `#4` allocation counts):

| zoom | tiles | loaded per tile (nodes/ways/areas) | allocs/tile/frame | step 4 allocs/tile/frame | step 4 ms/frame | step 4 share |
|------|-------|-----------------------------------|-------------------|--------------------------|-----------------|--------------|
| 14 | 4 | 96 / 3018 / 1802 | 6158 | 5415 | 2.61 | 25 % |
| 15 | 4 | 101 / 3030 / 8812 | 27153 | 26445 | 6.44 | 41 % |
| 16 | 16 | 680 / 2035 / 2747 | 10442 | 8248 | 2.03 | 26 % |
| 17 | 56 | 165 / 1153 / 917 | 3516 | 2757 | 0.58 | 21 % |

Step 4 allocates very close to **3 allocations per loaded area** in every band view
(5415/1802, 26445/8812, 8248/2747, 2757/917), and the fixed view shows the same ratio
(52939/17629 = 3.0). The three per-area allocations are the range vector
(`MapPainter.cpp:1224`), the `std::function` visitor (`:1266`) and the per-ring border style
vector (`:1110`).

### Prepared-entry reference dump

`baseline-prepared-areas.txt` (this directory) holds one line per prepared area entry of the
fixed view (726 lines), recording type name, outer/inner role, object file offset, range
size, clipping count and the transformed coordinates of the entry:

```
PERF_DUMP_PREPARED=<this dir>/baseline-prepared-areas.txt ./build/Tests/PerformanceTest \
  --driver noop --start-zoom 15 --end-zoom 15 --draw-repeat 1 --load-repeat 1 \
  --icons libosmscout/data/icons \
  maps/Dortmund stylesheets/standard.oss 51.514 7.463 51.510 7.470
```

The dump was produced with a temporary probe (`PreparedDumpPainter` in
`Tests/src/PerformanceTest.cpp`, marked `TEMP-PROBE`, reverted before this change was
finished) because no shipped tool prints the prepared entries of a real view.
`AfterPreprocessingCallback` is used, so the dump is in sorted draw order.

Clipping coverage of that view: 717 entries without clippings, 5 with one clipping ring and
4 with two, so the clipping-ring rule of this change is exercised by real data.

Note on the coordinates: `CoordBuffer` is renumbered per frame, so the absolute indices of a
prepared range legitimately shift once fewer rings are transformed. The reference dump lists
the coordinates, which is the contractual part (geometry), not the buffer indices.

### Measurement stability and the A/B harness

Allocation counts are exact and reproducible (identical across runs, and identical between
the noop and the cairo runs of the same view).

Wall time on this machine is not stable: the same configuration reported frame times between
13 ms and 38 ms in consecutive runs while this change was applied, i.e. the box is loaded.
For the before/after comparison the same test binary is therefore run **alternately** against
two copies of the library:

```
cp build/libosmscout-map/libosmscout_map.so.1.1.1 /tmp/lib-<variant>/ && cp ... /tmp/lib-<variant>/libosmscout_map.so.1
LD_LIBRARY_PATH=/tmp/lib-before ./PerformanceTest ...   # library of the pre-change code
LD_LIBRARY_PATH=/tmp/lib-after  ./PerformanceTest ...   # library of this change
```

The loader needs the SONAME name `libosmscout_map.so.1`; a first attempt that copied only the
versioned file silently kept loading the build-tree library (both variants then reported the
same allocation count), which is why the harness is validated by the allocation count before
any timing is read: `before` reports 53925 and `after` 1022 allocations per frame.

### Where the baseline allocations sit (from `TODO.md` and the earlier probe bisect)

- about 17631 of the 53901 frame allocations: one `std::vector<CoordBufferRange>` per loaded
  area (`MapPainter.cpp:1224`), including the 16903 areas that are never prepared;
- about 35295: the per-ring work in `VisitRings`/`PrepareAreaRing` (style lookups, one border
  style vector per visited ring at `:1110`, the `std::function` visitor built at `:1266`);
- about 441: step 2 `CalculatePaths`; 30: step 6; the rest: label steps.

## 2. Reused scratch storage for the area preparation

Change: `PrepareArea` fills a reused painter member `ringCoordRanges` instead of the per-area
`std::vector<CoordBufferRange> td(area->rings.size())`, and `PrepareAreaRing` passes the
reused `borderStyles` member to `GetAreaBorderStyles` instead of building a vector per visited
ring. `TransformAreaRing` (`MapPainter.h`, `MapPainter.cpp`) is the single site that turns a
ring into a coordinate range, and a ring stored as segments is now collected into the reused
`ringNodes` member. The reused stores are cleared per use (`ringCoordRanges.assign(...)`,
`borderStyles` is cleared by `GetAreaBorderStyles`, `styleConfig.cpp:1386`; `ringNodes.clear()`).

Evidence:

- Step 4 allocations on the fixed view: 52939 -> **32.2 per frame** (`161` for the 5-frame run).
- Frame allocations, fixed view: 53901 -> **994 per frame** (`4970` for the 5-frame run).
- Band views, step 4 allocations per tile per frame: z14 5415 -> 6.7, z15 26445 -> 6.8,
  z16 8248 -> 4.9, z17 2757 -> 6.0, i.e. constant and no longer proportional to the loaded
  area count (requirement: area preparation does not allocate per loaded area).
- Band views, frame allocations per tile per frame: z14 6158 -> 750, z15 27153 -> 715,
  z16 10442 -> 2198, z17 3516 -> 763 (the remainder is the label steps, untouched here).
- Review of the leak paths: every read site re-checks validity, and no site keeps a reference
  into a reused store across a call. The only store whose contents outlive the call is the
  prepared entry itself, which copies `CoordBufferRange` by value (a handle with
  `coordBuffer`/`start`/`end`, `Transformation.h:383`) and appends clipping ranges to
  `AreaData::clippings`.

## 3. Non-allocating ring visitor

Change: the visitor passed to `Area::VisitRings` is built from a single pointer to a local
`RingContext` struct, so the closure fits the `std::function` small buffer (the previous
closure captured seven values, about 56 bytes). The visitor's return value semantics are
unchanged (`true` for ignore-typed rings and for accepted rings, `false` otherwise, which
`Area.cpp:627` uses as the descend signal).

Evidence:

- Step 4 allocations of the fixed view dropped from 52939 to 32.2 per frame; the pre-change
  probe attribution (17631 for the range vector, 35295 for the visit) means the remaining
  number is consistent with "no per-area closure allocation". The fallback of design D2
  (templated visitor overload in the core library) was not needed.
- The prepared entry set of the fixed view is unchanged (section 4.4), which includes nested
  rings outside the viewport, i.e. the visit still descends as before.
- The `AreaData` clipping context uses the same single-pointer capture, so the clipping
  lookup does not allocate per prepared ring either.

## 4. Styling and visibility decided before the transform

Change: `PrepareAreaRing` resolves the fill and border styles and runs
`IsVisibleArea(projection, ring.GetBoundingBox(), borderWidth/2)` before any transform, and
`TransformAreaRing` is called only for rings that pass both. Emission order and the
`areaData.push_back` sites are unchanged, and ignore-typed rings (clipping rings) are
transformed unconditionally in a pre-pass in `PrepareArea`, because the ring they clip is
prepared before they are visited (the visit is breadth first by nesting depth).

Evidence:

- **Accepted set and geometry identical**: the prepared-entry dump of the fixed view after the
  change is byte-identical to `baseline-prepared-areas.txt` (`diff` -> 0 lines), for all 726
  entries, including their transformed coordinates, their clipping ranges and their order.
- **Clipping rings keep geometry**: 9 of the 726 entries carry clipping ranges in the fixed
  view, and those ranges are part of the identical dump; the synthetic test
  `Clipping rings of a drawn area keep their geometry` additionally compares the clipping
  range against an independently computed transform of the inner ring.
- **Order unchanged**: `areaData` is filled in the same ring visit order and sorted by the same
  `AreaSorter` + `std::stable_sort` (`MapPainter.cpp:2121`), which the order test covers for
  equal-comparing areas.
- **Invisible and unstyled rings contribute nothing**: two synthetic tests assert that the
  visible area's range starts at the same coordinate buffer index with and without a
  large invisible / unstyled area in front of it (see section 5.2 for the sensitivity check).

## 5. Unit tests

`Tests/src/MapPainterAreaPreparationTest.cpp`, 6 test cases, 135 assertions, registered in
`Tests/CMakeLists.txt` and `Tests/meson.build`.

| requirement (spec) | test |
|--------------------|------|
| Area preparation does not allocate per loaded area | `Area preparation allocates a constant amount per frame` (16 vs 512 loaded areas, second frame measured) |
| Area preparation does not allocate per loaded area | `Loaded areas that are not prepared do not add allocations` (same 16 prepared areas, with and without 512 loaded areas) |
| Styling and visibility are decided before geometry is transformed | `Rings outside the viewport are not transformed` |
| Styling and visibility are decided before geometry is transformed | `Unstyled rings are not transformed` |
| Styling and visibility are decided before geometry is transformed | `Styled and visible rings keep their transformed coordinates` (coordinates compared against an independent `TransformArea`, so buffer indices may differ) |
| Clipping rings keep valid geometry | `Clipping rings of a drawn area keep their geometry` |
| Prepared areas, draw order and rendered output are unchanged | `Prepared areas keep the set and the order of the loaded areas` (32 equal-comparing areas, two consecutive frames) |

The allocation tests use a counting `operator new`/`operator delete` in the test binary, the
same approach as `Tests/src/PerformanceTest.cpp`, disabled in sanitizer builds
(`AREA_PREP_HAVE_ALLOCATION_COUNTER`).

### Sensitivity of the tests

Checked by building the test binary against the pre-change library (`git stash push` of the two
library files, rebuild, run, restore) - 131 of 135 assertions passed, i.e. the intended four
failed:

```
Tests/src/MapPainterAreaPreparationTest.cpp:452: REQUIRE( small.first<=32 )
  allocations for 16 loaded areas: 35 / for 512 loaded areas: 1027
Tests/src/MapPainterAreaPreparationTest.cpp:510: REQUIRE( heavy.first<=light.first+4 )
  allocations without unstyled areas: 36
Tests/src/MapPainterAreaPreparationTest.cpp:557: REQUIRE( A[0].coordRange.GetStart()==controlStart )   # unstyled ring
Tests/src/MapPainterAreaPreparationTest.cpp:592: REQUIRE( A[0].coordRange.GetStart()==controlStart )   # invisible ring
```

The geometry, clipping and order tests pass before and after by design: they are regression
guards for the "unchanged" requirement, not sensitivity tests.

## 6. Verification

### 6.1 CMake build

`cmake --build build` (Release, Ninja): exit 0, no errors. No warning in any changed file
(`grep warning: /tmp/build-final.log` has no hit for `MapPainter.cpp`, `MapPainter.h`,
`MapPainterAreaPreparationTest.cpp`). The 25 warnings of the full build are in untouched files
(`libosmscout-client-qt`, and the pre-existing `MapPainterOpenGL.cpp:464` `lineOffset` warning
recorded by the previous change). The single `error` match in the log is the vendored
`if (size == -1l) goto error;` line of a warning text, not a build error.

### 6.2 Meson build

```
meson compile -C build-meson MapPainterAreaPreparationTest   -> ok, no warnings
meson test -C build-meson "Check MapPainterAreaPreparation compilation"
  -> 1/1 OK
```

The first `meson compile` call reported "target not found" because the build directory had not
been regenerated after `Tests/meson.build` changed; `meson test` regenerated it and the target
builds warning-free.

### 6.3 Test suite

```
cd build && QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR=<src>/Tests TESTS_TMP_DIR=<build>/Tests \
  ctest -j 2 --output-on-failure
-> 100% tests passed out of 117 (58.7 s)
```

117 instead of 116 because `MapPainterAreaPreparationTest` is new. Includes
`MapPainterRouteTest`, `MapPainterShieldTest`, `MapPainterFrameBuffersTest`, the SVG/Cairo/
Qt/Skia drawing and text metric tests, the stylesheet checks and `PerformanceTest`. No test
regressed and no golden output changed.

### 6.4 Linters

Uncrustify 0.83.0_f (the version `scripts/format-check.sh` requires):

| file | drift vs uncrustify at HEAD | working tree |
|------|------------------------------|--------------|
| `libosmscout-map/src/osmscoutmap/MapPainter.cpp` | 243 lines | 222 lines |
| `libosmscout-map/include/osmscoutmap/MapPainter.h` | 50 lines | 47 lines |
| `Tests/src/MapPainterAreaPreparationTest.cpp` (new) | - | 0 lines |

The new file is formatted with the project configuration, and the two library files are
*below* their HEAD drift, so no added line is flagged. The remaining hunks inside the rewritten
region are pre-existing ones (`double offset=0.0;`, the blank line before `size_t
areaCount=0;`) that are also present in the HEAD drift. The block-level realignment of the
member declarations in `MapPainter.h` follows the tool, as the previous change did.
Note: `.uncrustify` wants right-attached pointers/references while `guidelines/CodeStyles.md`
documents left-attached; the added lines follow uncrustify where it rewrites them.

clang-tidy (`.clang-tidy`, `clang-tidy -p build`):

- Findings in the changed region of `MapPainter.cpp` are 10
  `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access`, 2
  `cppcoreguidelines-pro-bounds-pointer-arithmetic`, 9 `misc-include-cleaner` and one
  `cppcoreguidelines-avoid-magic-numbers` - all categories that are already present in
  unchanged parts of the same file (120 / 14 / 85 / 55 findings respectively).
- The only category that the new code introduced was `modernize-use-designated-initializers`
  (the two context structs). Fixed by initializing them with designated initializers, which
  also documents the six-pointer aggregate; the four remaining findings of that category are
  pre-existing. This is the only place where the change follows clang-tidy over the local
  convention of positional aggregate initialization.
- `MapPainterAreaPreparationTest.cpp`: clang-tidy cannot analyse it in this environment because
  the compile database was produced by GCC; same limitation the previous change recorded.

### 6.5 Measurement after the change

Fixed view, same binary, alternated against the two library builds, `--draw-repeat 5`:

```
cycle  noop step 4 ms/frame        noop step 4 allocations/frame
       before   after             before     after
1       8.47     6.75             52939      32.2
2      13.49     8.38
3      14.73     8.23
4      12.57     7.32
5      11.57     6.74
median 12.57     7.32   (1.7x)                (1643x fewer)
```

`after` is faster in all five cycles. Cairo, same view: step 4 12.51/14.05/11.94 -> 7.91/7.66/
7.03 ms per frame, frame allocations 57544 -> 4637 per frame.

Band views (noop, small tile box, `--draw-repeat 3`, per tile per frame):

| zoom | step 4 ms before -> after | step 4 allocs before -> after | frame allocs before -> after |
|------|---------------------------|-------------------------------|------------------------------|
| 14 | 1.15 -> 0.65 (1.8x) | 5415 -> 6.7 | 6158 -> 750 |
| 15 | 4.08 -> 2.52 (1.6x) | 26445 -> 6.8 | 27153 -> 715 |
| 16 | 1.25 -> 0.70 (1.8x) | 8248 -> 4.9 | 10442 -> 2198 |
| 17 | 0.41 -> 0.25 (1.6x) | 2757 -> 6.0 | 3516 -> 763 |

No zoom in the band regresses; the label-dominated remainder at z16/z17 is untouched by this
change, as scoped in the design's Non-Goals.

Not usable as evidence: a byte comparison of `DrawMapSVG`/`DrawMapCairo` output for the fixed
view (the SVG files and the PNG md5 are identical before and after) - those demo binaries
render a nearly empty image for this database/stylesheet combination (992-byte PNG at
512x512), so the equality says nothing. The prepared-entry dump and the test suite carry the
output-identity evidence instead.

### 6.6 TODO.md

The `ProcessAreas` scratch finding is closed with the measured numbers, and the remaining part
of the bullet was split into (a) the resolved area entry plus the still open
`AreaData::clippings` conversion, and (b) the label scratch finding, which now records the
label steps as the largest remaining allocation source. The other findings (label pipeline,
font cache, `MapService::AddTileDataToMapData`, way paths, import, data loading) stay open.
