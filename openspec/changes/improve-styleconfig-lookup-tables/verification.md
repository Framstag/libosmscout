# Verification

Change: `improve-styleconfig-lookup-tables` (spec `style-configuration`).

Verified on 2026-10-03 on branch `improve-styleconfig-lookup-tables` (HEAD `616f9f9d0`, base `origin/master`
`26aec174e`), Linux, GCC, Release builds.

## 1. The rendering output is unchanged

Two states of this branch were built and used to render every symbol of the shipped `standard.oss`:

- **base**: `git worktree add /tmp/base-12 67884c96c` (the dense tables of `origin/master` plus the new test
  file), configured minimally (`-DOSMSCOUT_BUILD_MAP_QT/OPENGL/SKIA/AGG=OFF`, `-DOSMSCOUT_BUILD_CLIENT*=OFF`,
  `-DOSMSCOUT_BUILD_TESTS=OFF`) and built with `cmake --build /tmp/base-12-build --target SymbolsAll`;
  rendered with `--output /tmp/render-before`
- **after**: the branch's own build (`build/`), rendered with `--output /tmp/render-after`
- **control**: a second run of the branch's build into a fresh directory (`--output /tmp/render-check`)

```text
SymbolsAll --stylesheet stylesheets/standard.oss --ost stylesheets/map.ost --sheet --output <dir>

832 files written by each run (414 symbols x PNG+SVG, plus patterns.png/symbols.png/symbols.svg)
base vs after: 832 common files, 0 differing, no file only in one of them
control run:   832 files, DrawMap* absent
```

Each file was compared by `sha256sum`.

Note on five further files: `/tmp/render-before` and `/tmp/render-after` also held
`DrawMapCairo.png`, `DrawMapOpenGL.png`, `DrawMapQt.png`, `DrawMapSVG.svg` and `DrawMapSkia.ppm`. Those are not
written by `SymbolsAll` (the string `DrawMap` does not occur in `Demos/src/SymbolsAll.cpp`, and the control run
into an empty directory produced none of them); they are leftovers of a different tool in the reused temporary
directories and are unrelated to this change. The comparison above therefore covers every file the two
`SymbolsAll` runs wrote.

## 2. Build cost of the shipped stylesheets (task 3.2)

From the test (`Tests/src/StyleConfigLookupCostTest.cpp`, case "The build cost of the shipped stylesheets is
reported"):

```text
ctest -R StyleConfigLookupCost --output-on-failure
  shipped standard.oss: 17.1904 ms, repeated 13.3029 ms
  slots 152560, type condition evaluations 7804,
  table bytes 3845544, type set bytes 615600
```

Per style family (slots prepared, from `BuildDiagnostics::familySlots` and from
`Demos/SymbolsAll --list`, which prints the same figures to stderr so its stdout stays a plain symbol list):

```text
  family nodeText: 59640          family areaBorder: 12000
  family areaText: 25740          family wayPathSymbol: 9020
  family wayLine: 31540           family areaFill: 7560
  family nodeIcon: 4680           family areaIcon: 1180
  family wayPathText: 1140        family wayPathShield: 60
  family areaBorderText/Symbol, routeLine, routePathText: 0
```

## 3. The cost no longer follows the defined type count (tasks 1.2, 2.2, 3.1)

**Synthetic configuration** (the test's own A/B: the same style sheet against a type configuration with one
area type and against one extended by 50 types the sheet does not reference):

```text
                                    before (dense tables)      after
prepared slots                      790 -> 4740                30 == 30
type condition evaluations          300 -> 1800                4 == 4
table bytes                         21144 -> 126744            792 == 792

type set bytes (reported separately, `*TypeSet` vectors are
type-count sized until TODO §20)    2688 -> 13888
```

The before figures were measured during implementation on the working-tree state that already carried the
diagnostics but still the dense tables; that state is not a commit, so they are quoted rather than
reproducible from `git`. The after figures are asserted by the case "Defined but unreferenced types add no
build work".

**Shipped type configuration against the parked set** (`/tmp/map-unparked.ost`: the shipped `stylesheets/map.ost`
with its `PARKED-NEW-TYPE` blocks uncommented into a temporary copy whose sibling `.ost` files are symlinked,
1489 defined types against 638; the same style sheet and the same parked style rules in both runs, so only the
defined type count moves). `SymbolsAll --list`, end to end, median of five runs, measured in one session
against the two binaries:

```text
                            base (dense, 67884c96c)      after (616f9f9d0)
shipped map.ost, 638 types  median 168.4 ms (122.0-238.1)  median 28.8 ms (27.3-42.8)
parked set, 1489 types      median 234.7 ms (228.9-305.5)  median 56.7 ms (50.9-69.9)
slope for +851 types        +66.3 ms                       +27.9 ms
```

Caveat: the base binary was built with fewer backends enabled (Qt, OpenGL, Skia and AGG off) while the after
binary has them; the measured path (type definition parse, style sheet load, symbol listing) is the same code in
both, and the comparison is same-session and same-machine.

An earlier observation of the same slope with the branch's own build directory before and after the table
change: 103.4 ms (638 types) and 232.9 ms (1489 types) before, 35.5 ms and 60.9 ms after.

The remaining +27.9 ms slope is the `*TypeSet` vectors (still one pointer per defined type per level, TODO §20)
plus parsing the much larger `.ost` file; both are outside this change.

## 4. Resolution cost per call (task 3.3)

From the case "The per-call cost of resolving a style is reported" (200 000 resolutions each, one projection
and one feature value buffer, 51 defined types of which one is referenced):

```text
referenced type:   11.4203 ns per resolution (200000 hits)
unreferenced type:  1.87962 ns per resolution (200000 misses)
```

An unreferenced type is cheaper than a referenced one, as designed: it takes the "not referenced" branch of the
position table instead of walking a level's selector list. The pre-change figure for this path was not
measured (it needs a base build of the same test); the direction is bounded by the end-to-end figures in
section 3.

## 5. Suites, formatter and linter (tasks 4.2-4.4)

```text
CMake   ctest -j 2 --exclude-regex PerformanceTest                 106/106 passed
        (QT_QPA_PLATFORM=offscreen, TESTS_TOP_DIR, TESTS_TMP_DIR)
Meson   meson compile -C build-meson                               187 targets
        meson test --timeout-multiplier 2 -C build-meson            145/145 OK
        without the multiplier `Check threaded database` reaches the 30 s
        limit at 34 s (AGENTS.md documents the multiplier; the CMake run of
        the same test passes at 32 s)
uncrustify 0 hunks change a line this branch added; the new test file is fully clean;
           the 3 hunks in StyleConfig.cpp and 3 in SymbolsAll.cpp are pre-existing drift
clang-tidy findings on lines this branch added, after the fixes:
           cppcoreguidelines-pro-bounds-avoid-unchecked-container-access 15 (repo-wide class: the
           whole tree indexes vectors with `operator[]`; 233 findings in the same run of this file alone)
           misc-use-internal-linkage 3 (pre-existing helpers whose signature line this change touched)
           readability-function-cognitive-complexity 1 (same pre-existing function)
           readability-redundant-typename 1 (false positive: the `typename` is required for the
           dependent `Table::value_type` in the return type of `LookupRow`)
```

Fixed while verifying: `static` on the three new file-local helpers, direct `<cstdint>` and
`osmscout/TypeConfig.h` includes, explicit parentheses in `TypeSetBytes`, if/else instead of the nested
conditional in `CalculateUsedTypes`.

## 6. PerformanceTest A/B (task 6.2)

The base worktree was reconfigured with the full library set (Qt, OpenGL, Skia, SVG, Cairo, client, import)
and `cmake --build /tmp/base-12-build --target PerformanceTest`, so both states can run the same command:

```text
xvfb-run -a ctest -R PerformanceTest

base (67884c96c)  35 `PerformanceTest-*` sub-tests present, all passing; the four further perf targets
                  (CachePerformanceTest, TypeResolutionPerformanceTest, NumberSetPerformanceTest,
                  ReaderScannerPerformanceTest) were not built in this worktree and report "Not Run"
main (HEAD)       39 tests, all passing

the 35 common sub-tests: all pass in both states
  total                    base 8.41 s   main 6.12 s
  noop   7 tests           base 0.77 s   main 0.34 s
  cairo  7 tests           base 0.76 s   main 0.46 s
  Qt     7 tests           included in the totals
  opengl 7 tests           base 3.47 s   main 3.80 s

printed figures of `PerformanceTest-cairo-standard.oss`:
  base: DB total 0.61 s, Draw allocs 113, Map total 0.34 s
  main: DB total 0.67 s, Draw allocs 113, Map total 0.23 s
```

The draw allocation count is identical (113) and no sub-test changed its status; the per-backend times differ
by less than the run-to-run spread of these tests (the OpenGL group even reads slower in the newer build). The
load-time slope comparison is in section 3.

## 7. Limitations

- The pre-change *slot*, *evaluation* and *byte* counts come from the implementation session (see section 3),
  not from a commit: the diagnostics and the dense tables were never committed together.
- The pre-change per-call resolution figure was not measured.
- `MapDataBudgetTest` is not registered in this configuration at all (`ctest -R MapDataBudgetTest` finds no
  test): it belongs to the open `map-data-memory-budget` change, not to `origin/master`, so there is no figure
  of it to compare here.
