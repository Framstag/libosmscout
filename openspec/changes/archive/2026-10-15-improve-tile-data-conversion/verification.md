# Verification

Evidence for the tasks of `improve-tile-data-conversion`. Sections are appended by the task group that
produced them.

## 1. Pre-change conversion baseline (task 1.1)

### Command

Options come before the positionals; the parser rejects them after the six positional values.

```
./build/Tests/PerformanceTest --start-zoom 15 --end-zoom 15 --driver noop \
  --font libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf \
  --icons libosmscout/data/icons/svg/standard \
  maps/Dortmund stylesheets/standard.oss 51.55 7.40 51.48 7.55
```

Workload: a local, untracked Dortmund database (`maps/Dortmund`, the repository does not track it) at
one magnification level (zoom 15). The tile area holds 180 tiles and `PerformanceTest` converts once
per viewport, so the run performs **180 conversions** of 16 tiles each. `--driver noop` keeps rendering
out of the number.

### Temporary instrumentation (reverted before the implementation)

`libosmscout-map/src/osmscoutmap/MapService.cpp` and `Tests/src/PerformanceTest.cpp` carried a
measurement-only edit, reverted with `git checkout --` before the conversion was rewritten:

- `MapService.cpp`: one `log.Info()` line per conversion with the tile count, the insertions (the sum of
  the six kinds' `GetDataSize()` over all tiles), the distinct objects (the sizes of the six hash maps
  after the insertion loop) and the deduplication duration (`uniqueTime`, whose output was a
  commented-out stream before), plus one line with the copy duration (`copyTime`) that already existed.
- `PerformanceTest.cpp`: the allocation counter the file already installs (`GetAllocationCount()`,
  replacing global `operator new`) brackets the `AddTileDataToMapData` call, so the allocated blocks of
  one conversion are reported.

### Level totals over 180 conversions (pre-change)

| metric | total | per conversion |
|---|---|---|
| insertions (objects inserted into the six maps) | 7,666,618 | 42,592 |
| distinct objects of the result | 2,481,311 | 13,785 |
| insertions / distinct | 3.09 | |
| allocated blocks | 2,483,018 | 13,795 |
| allocated blocks - distinct objects | 1,707 | 9.5 |

The largest single viewport: 73,968 insertions for 23,388 distinct objects (ratio 3.16), deduplication
9.24 ms.

Two conclusions, both of which the design assumed:

- The conversion inserted **3.09 objects per object of the result**, and its allocation was exactly
  **one heap block per distinct object** (2,483,018 / 2,481,311 = 1.001, plus a constant of 9.5 blocks
  per conversion).
- Per conversion the pre-change cost is deduplication 6.4 ms + copy 1.9 ms in the fastest run, a
  significant part of a 16 ms frame budget.

### Reproducibility

The counters are exactly reproducible: five consecutive runs of the recorded command report
`insertions=7666618`, `distinct=2481311` and `blocks=2483018` **identically**.

The durations are not, on this machine:

| run | deduplication (ms) | copy (ms) |
|---|---|---|
| 1 | 1,149.4 | 337.1 |
| 2 | 2,567.0 | 997.2 |
| 3 | 2,693.4 | 881.1 |
| 4 | 2,400.4 | 873.8 |
| 5 | 2,787.4 | 936.1 |

Spread of runs 2-5 is about 15 %; run 1 is 2.4x faster than the median, most likely because the machine
was still idle at that point. **Deviation from task 1.1**: the "reproduce each number within 10 %"
criterion is met by the three counters and not by the two durations across runs. The record therefore
uses the median of five runs (deduplication 2,567 ms, copy 881 ms for the level, i.e. 14.3 ms and
4.9 ms per conversion) and treats the counters as the reproducible part of the baseline. This is the
reason design decision D4 compares the baseline and the conversion **inside one run** instead of
against a number recorded earlier; a cross-run duration comparison would not be a verification here.

## 2. Test roster before the change (task 1.2)

| build system | command | entries | file |
|---|---|---|---|
| CMake | `ctest -N` in `build/` | 137 | `verification-roster-cmake.txt` |
| Meson | `meson test --list` in `build-meson/` | 137 | `verification-roster-meson.txt` |

The two lists do not share a naming convention (CMake names the test after the target, Meson takes a
prose string), so task 5.2 compares them by entry count plus a search for the new names in both lists —
the same limitation the `Nothing checks that both build systems register the same tests` entry of
`TODO.md` describes.

## 3. The mechanism, and the measurement that chose it (tasks 2.1 - 2.3, 3.1 - 3.5)

### What was implemented first, and why it was dropped

The design first chose to merge the ascending groups of the tiles through a heap over their heads: the
implementation added group bookkeeping and a read guard to `TileData<O>` in
`libosmscout-map/include/osmscoutmap/DataTileCache.h`, took the tiles' guards in the conversion, and
merged one source data file at a time. `Tests/src/TileDataCacheTest.cpp` covered the group bookkeeping
and the two read paths.

Measured against the conversion it replaces, that merge **lost on exactly the tile sets this change is
about** (the numbers are the fastest of 20 runs per side, test database, one run):

| level | tiles | distinct | objects/reference | merge / baseline |
|---|---|---|---|---|
| 15 | 56 | 1619 | 12.25 | 1.16 |
| 16 | 56 | 1125 | 13.07 | 2.32 |
| 17 | 90 | 1105 | 13.23 | 2.29 |
| 16 | 16 | 619 | 7.00 | 1.09 |
| 18 | 30 | 53 | 6.19 | 0.90 |
| 12 | 90 | 42 | 3.60 | 0.48 |

The merge performs a heap operation per **object reference** (`O(log runs)`, a 24-byte record moved per
swap), while the previous implementation performed a hash lookup per reference and allocated only per
distinct object. Since a large tile set repeats its objects many times, the heap work grew where the
hash work did not.

The design was therefore changed (approved as option A of the decision record): the merge, the
`TileData` group bookkeeping and its guard were reverted, and the deduplication is a **flat hash table
of the offsets already seen** per source data file (`FileOffsetSet` in `MapService.cpp`), which
allocates its table when it grows rather than a node per object, and which is looked up once per
reference like the maps it replaces.

The intermediate step was measured too: a node-based `unordered_set` per source (the design's D1-B
alternative) reached 0.39 - 1.18 of the baseline, losing on the large tile sets because it rehashes
from an empty table while the maps it replaces pre-allocate 10000 buckets.

### What the tests now cover (tasks 3.1 - 3.5, 5.4)

`Tests/src/TileDataConversionTest.cpp`, nine cases on a real view of `Tests/data/testregion`:

| property | assertion |
|---|---|
| every object once | the sorted result offsets of a kind equal the offsets the tiles hold, and the result holds no offset twice |
| the source grouping | no object of the optimized data file precedes an object of the regular data file of its kind |
| reproducible order | the same tile list converts to the same sequence twice |
| duplicates | a tile list that names every tile twice converts to the same sequence, and the tile set holds more object references than the result holds objects (so the deduplication is exercised) |
| the two offset spaces | a tile whose regular and optimized way data (and area data) hold the same objects converts to both |
| the restricted entry point | only the requested types, each once, exactly the matching objects of the tiles, all of them in the unrestricted result |
| phase reporting | a threshold of zero reports every phase by name, one warning each |
| no warning below the threshold | an unreachable threshold logs nothing, and the default threshold is 20 ms |
| the allocation | at most one block per distinct object plus a budget of 256, and converting a tile set whose objects are all carried twice allocates no more than converting it once |

The allocation case is the one that does not depend on timing, and the "carried twice" part is the part
that pins the property of the spec: the allocation may not grow with the references.

**Not covered by the unit test**: the route source. Its conversion is the same code path with the same
arguments, but the view's routes are not loaded by the stylesheet-driven loader of the test.

### Where the test data comes from

The conversion can only be tested with objects the library read itself: a file offset is assigned
inside `Node::Read`/`Way::Read`/`Area::Read` from the scanner position (`Node.cpp:51`, `Way.cpp:82`,
`Area.cpp:187`) and there is no setter anywhere in the API, so a test cannot fabricate an object with a
chosen offset. `TileDataConversionTest` therefore loads a real view of `Tests/data/testregion` and
reads the objects of that view back through `CopyData`.

The view is derived from `Tests/data/testregion.poly`, not from the bounding box the database stores:
the stored box describes the file the data was extracted from (the whole Czech Republic,
48.81-50.89 / 12.75-18.68) while the region itself is at 50.40-50.45 / 14.53-14.61, so a view from the
stored box loads no data at all.

## 4. The speed gain (task 5.3)

### On the test database, per tile set

`Tests/src/TileDataConversionPerformanceTest.cpp` holds the previous conversion as its baseline,
converts eight tile sets through the baseline and through the conversion in one run, and keeps the
fastest of 20 runs per side. Measured (the margins are 0.9 per tile set and 0.8 overall):

| level | tiles | objects | conversion / baseline |
|---|---|---|---|
| 15 | 56 | 1619 | 0.42 |
| 16 | 56 | 1408 | 0.62 |
| 16 | 16 | 962 | 0.49 |
| 17 | 90 | 1124 | 0.80 |
| 17 | 30 | 802 | 0.61 |
| 18 | 90 | 777 | 0.82 |
| 18 | 30 | 604 | 0.63 |
| 18 | 12 | 554 | 0.51 |
| **over all tile sets** | | | **0.62** |

Each tile set is between 1.2x and 2.4x faster, and the conversion needs 0.62 of the baseline over all
of them. The three runs of the finished test all pass.

### End to end, on a production-shaped database

The recorded baseline command of section 1, run again with the same temporary instrumentation on the
conversion (a `log.Info()` line with the summed phase durations) and the same allocation bracket:

| metric | pre-change | after | ratio |
|---|---|---|---|
| conversion total over 180 conversions | 1,149.4 ms (run 1); 3,448 ms (median dedup + copy) | **130.5 ms** | 0.11 (run 1) / 0.038 (median) |
| allocated blocks over 180 conversions | 2,483,018 | **3,547** | 0.0014 |
| allocated blocks per conversion | 13,795 | **19.7** | |

The production-shaped view holds about 13,800 distinct objects in 16 tiles per conversion and repeats
them 3.1 times; there the conversion is about **9x to 26x** faster and allocates about 700x fewer
blocks, i.e. its allocation no longer follows the object count at all. The difference to the small test
tile sets above (1.2x - 2.4x) is the reason the test asserts over several tile sets instead of one: the
previous implementation's fixed cost (six maps with 10,000 buckets each) is amortised over few objects
and its per-object allocation dominates only at scale.

## 5. Roster of both build systems after the change (task 5.2)

| build system | entries | delta against section 2 |
|---|---|---|
| CMake (`ctest -N`) | 139 | `TileDataConversionTest`, `TileDataConversionPerformanceTest` |
| Meson (`meson test --list`) | 139 | `Check tile data conversion`, `Check tile data conversion performance` |

The delta is exactly the two intended tests, in both build systems. `TileDataCacheTest` appears in
neither list: it was deleted with the merge implementation (section 3).

## 6. Rendered output (task 6.4)

The conversion's order changed from the iteration order of the hash maps to the order of the tile list,
and the painter's preprocessing stable-sorts its prepared ways and areas
(`MapPainter.cpp:2375-2379`), so the `MapData` order is a tiebreaker in the rendered image. The same
view was rendered through `Demos/DrawMapCairo` before and after the change (Dortmund,
51.5136 7.4653, the same stylesheet and font):

| magnification | differing pixels | share of the 1920x1080 frame |
|---|---|---|
| 4000 | 1,340 | 0.065 % |
| 8000 | 2,307 | 0.111 % |
| 16000 | 3,501 | 0.169 % |

The peak deviation of a single pixel at magnification 8000 is 63 % of full intensity, so the
differences are not antialiasing noise at the edges of identical shapes: a few objects or labels are
drawn in a different stack position where two objects share a sort key. This is the expected
consequence of decision D2 - the previous order was the iteration order of a hash map, which no caller
could control and which already differed between standard libraries - and it is recorded here instead
of being treated as a failure. A user-visible difference is therefore possible where objects overlap
and share a sort key; no stylesheet can be singled out without inspecting the difference image, so this
stays a known, measured consequence of the change rather than a resolved defect.

Both renders report the same pre-existing warnings (`ERROR while loading pattern image
'natural_scrub'`, `'landuse_cemetery'`), which is the pattern wiring issue tracked separately in
`TODO.md`: the demo sets icon paths but not pattern paths.

## 7. Build and test configuration (tasks 6.1 - 6.3)

| check | result |
|---|---|
| `cmake --build build -j 6` | no error, and no warning in any changed file (the tree's warnings are pre-existing: Qt deprecations in `MapWidget.cpp`/`InputHandler.cpp` and doxygen config notices) |
| `ctest -j 4 --output-on-failure` in `build/` | **139 of 139 pass** |
| `meson compile -C build-meson` | no error; only the pre-existing client-qt deprecation warnings |
| `meson test --timeout-multiplier 2 -C build-meson` | **Ok 139, Fail 0** |
| `build-asan` (AddressSanitizer + UndefinedBehaviorSanitizer), `ctest -R TileDataConversionTest` | passes, 7.9 s |

`TileDataConversionPerformanceTest` is a performance test and has `PerformanceTest` in its name, so the
sanitizer configuration's `--exclude-regex "PerformanceTest"` excludes it without a change to the CI
workflow. The allocation counter of the tests disables itself when the binary is built with a sanitizer,
because the sanitizer runtime defines the global `operator new` and `delete` itself and a replacement
here would collide with them at link time (seen with the MemorySanitizer runtime, which defines the same
operators as the AddressSanitizer one); `TileDataConversionTest` then reports that the allocation bound
is not checked and skips only that case. In Meson the performance comparison is additionally not
registered when the build is instrumented for coverage, see section 9.

## 8. Defects found while implementing (of the tests, not of the library)

1. **A live read guard excluded every other accessor of the same tile.** The first version of the
   `TileData` test deadlocked on `GetDataSize()` while a guard existed (non-recursive tile mutex). The
   guard was documented and the callers were ordered accordingly; both the guard and the group
   bookkeeping were reverted with the merge (section 3).
2. **A tile list that names the same tile twice deadlocked the merge.** Each entry took its own guard
   for the same tile, and the second waited for the first. The merge ordered and deduplicated the tiles
   by address to fix it; reverted with the merge. The current conversion reads a tile through
   `CopyData`, which takes and releases the tile lock per call, so a repeated tile is simply visited
   twice and deduplicated by the seen-offset table. `TileDataConversionTest` converts a list that names
   every tile twice and requires the same result.
3. **A capturing log destination must be thread-safe.** `MapService` loads tiles on worker threads, so
   the capture of the test is written to from more than one thread; without a mutex the output string
   was corrupted, and the test failed in three different ways at random (a hang, a segmentation fault
   inside an unrelated assertion, and a failed assertion). The destination guards its output now.
4. **Two test expressions took `begin()` and `end()` from two different temporaries**
   (`std::set(OffsetsOf(x).begin(), OffsetsOf(x).end())`), which is undefined behaviour and produced the
   segmentation fault above; both now hold the vector in a local.

## 9. Defects of the change, found by CI after the first push

The three following defects broke a build or test job of the pull request and are fixed in it.

1. **The counting allocator collided with the MemorySanitizer runtime.** The guard of
   `TestAllocationCounter.cpp` recognized AddressSanitizer only, so the MemorySanitizer job compiled the
   replacement of the global `operator new` and `delete` and the link failed:
   `multiple definition of 'operator new(unsigned long)'; .../libclang_rt.msan_cxx-x86_64.a(msan_new_delete.cpp.o):
   first defined here` (eight of them, one per replaced operator). The guard now covers
   `address_sanitizer` and `memory_sanitizer`. Verified by preprocessing the file per sanitizer: only the
   plain and the UndefinedBehaviorSanitizer compilations still define the counting operators.
2. **`Logger::ERROR` did not compile with the Windows headers.** They define `ERROR` as a macro (value
   `0`), so `osmscout::Logger::ERROR` expanded to `osmscout::Logger::0` in `TileDataConversionTest.cpp`
   (`error C2589: 'constant': illegal token on right side of '::'` on MSVC, `expected unqualified-id
   before numeric constant` on MinGW). The test clears the macro after all of its includes, before the
   class that names the enumerator, with the same guard `Logger.h` itself uses; a header of the includes
   defines it late enough to survive the clear `Logger.h` performs. Verified by simulating the macro in the
   last header the test includes: with the guard the file compiles, without it the same error reproduces.
3. **The coverage build of the sonar job failed the performance comparison.** The job builds with
   `-Db_coverage=true` and runs every Meson test; the instrumentation slows the conversion under test - it
   lives in `libosmscout-map` - but not the hash map baseline, which lives in the standard library, so the
   comparison measured the instrumentation: a share of 4.0 to 5.8 instead of well below one (measured
   locally with `--buildtype debugoptimized -Db_coverage=true --unity on`, 196 assertions, 9 failed). The
   comparison is a performance test and is no longer registered in Meson when `b_coverage` is set; the
   binary is still built and `TileDataConversionTest` still runs there.
4. **MSVC failed to link `TileDataConversionTest`.** The test calls the lvalue overload of
   `TileData::SetData()` to put the regular ways and areas of a tile into its optimized data
   (`TileDataConversionTest.cpp:671`, `:682`); the library itself only calls the rvalue overload, so the
   lvalue one is not instantiated in the DLL and MSVC, which does not let a client instantiate a member of
   a dllimport class, reported `LNK2019: unresolved external symbol ... TileData<...>::SetData` twice and
   `LNK1120: 2 unresolved externals`. `DataTileCache.cpp` now instantiates `TileData<NodeRef>`, `<WayRef>`,
   `<AreaRef>` and `<RouteRef>` as a whole, so every member of the exported class template is part of the
   library. Verified by building and linking the test in `build-asan` and running it, which is the same
   class of failure the MSVC import library raises.
5. **Two pre-existing Meson tests timed out in the sonar coverage job.** `Check type resolution
   performance` and `Check threaded database` have no timeout of their own, so they run against Meson's
   default of 30 seconds, and the coverage build of a loaded CI runner needs more: they took 7.1 s and
   17.5 s on the idle runner that produced the pre-change record and more than 30 s on the loaded runners
   of the two later runs (`Check threaded database` alone measures 37.8 s in a local debug build). Both
   now declare `timeout: 120`, the headroom the other slow tests of the suite already have.
