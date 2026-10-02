# Tasks

## 1. The tree check and its fixture

- [x] 1.1 Write `scripts/check-vector-self-append.sh` so it scans the repository's source trees for
  appends whose receiver and whose argument's leading expression name the same object, prints both
  expressions for every finding and exits non-zero on a finding; accept optional path arguments so a
  caller can point it at a fixture (verification: the script exists, is executable, and
  `scripts/check-vector-self-append.sh libosmscout/src/osmscout/util/Transformation.cpp` reports the
  two closing appends of that file). Spec: *Optimization is independent of the storage state of the
  optimized sequence*.
- [x] 1.2 Add a fixture under `Tests/data/` that holds one self-append and one correct
  cross-container append, in a file extension the tree scan does not cover
  (verification: running the script against the fixture reports the self-append and not the
  cross-container append, and the script's tree scan reports nothing for the fixture directory).
  Spec: *Optimization is independent of the storage state of the optimized sequence*.
- [x] 1.3 Register the script as a test in both build systems, next to the existing signature check
  (`Tests/CMakeLists.txt:188` and `Tests/meson.build:14`), and make the test run the tree scan plus the
  fixture case (verification: `ctest -R VectorSelfAppend` and
  `meson test -C build-meson --list | grep -i vector` both name the test, and it fails on the
  unmodified tree because of the known sites). Spec: *Optimization is independent of the storage state
  of the optimized sequence*.

## 2. The append sites

- [x] 2.1 Change the two closing appends in `libosmscout/src/osmscout/util/Transformation.cpp`
  (`EnsureSimple`: the area closing point and the cut-off branch) so each appends a value that does
  not live in the sequence being grown (verification: the script no longer reports those lines, the
  translation unit's own Release compile command from `build/compile_commands.json` reports no
  possible-uninitialized-use warning, and `Tests/src/TransPolygonTest.cpp` cases pass). Spec: *The
  optimizer compiles without a possible-uninitialized-use diagnostic*.
- [x] 2.2 Change the three closing appends in `libosmscout/include/osmscout/util/Geometry.h`
  (`AreaIsSimple`, `AreaIsValid` for the outer and for the inner rings) the same way (verification:
  the script reports nothing for the header and the geometry tests in `Tests/` pass). Spec:
  *Optimization is independent of the storage state of the optimized sequence*.
- [x] 2.3 Change the closing appends of the two further sites the check found beyond the design's
  inventory: the coastline append in `libosmscout-import/src/osmscoutimport/WaterIndexProcessor.cpp`
  (its equality guard is kept), the ring-point append in
  `libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp` and the antimeridian coastline
  append in `BasemapImport/src/BasemapImport.cpp` (verification: the script reports nothing for the
  three files and the import library, the basemap import tool and the OpenGL painter compile). Spec:
  *Optimization is independent of the storage state of the optimized sequence*.
- [x] 2.4 Re-run the script over the whole tree and confirm the seven correct cross-container appends
  named in `proposal.md` are not reported (verification: `scripts/check-vector-self-append.sh` exits
  zero and its output names no site). Spec: *Optimization is independent of the storage state of the
  optimized sequence*.

## 3. Test coverage of the optimizer

- [x] 3.1 Add a case to `Tests/src/TransPolygonTest.cpp` that drives the area optimization with a ring
  whose drawn point count equals the storage capacity of the optimized sequence when it closes the ring
  (four well-separated points, tolerance below their spacing, through `TransformArea`) and asserts that
  the result keeps every point of the ring and is simple as a closed ring (verification:
  `./build/Tests/TransPolygonTest` passes; measured 2026-09-29 that this case cannot fail for the
  storage-state defect on the standard library used here, and that the closing decision keeps its own
  regression case - the pre-existing `Optimized area is still simple` case fails when the closing
  append is removed). Spec: *Optimization is independent of the storage state of the optimized
  sequence*.
- [x] 3.2 Add the complementary case with a ring that does not force growth and assert the same shape
  contract, so both storage states are covered (verification: both cases pass and the existing three
  simplicity cases stay green). Spec: *Optimized geometry is simple and closed*.
- [x] 3.3 Confirm the check detects a reintroduced self-append (verification: with the pre-change form
  of both `Transformation.cpp` appends temporarily restored, `scripts/check-vector-self-append.sh`
  reported both lines and exited 1 while the optimizer cases still passed - the measured limitation
  recorded in `design.md`; the temporary edit was reverted and the tree is clean afterwards). Spec:
  *Optimization is independent of the storage state of the optimized sequence*.

## 4. Documentation and bookkeeping

- [x] 4.1 State the convention in `guidelines/CodeStyles.md`: an append must not take a reference into
  the sequence it appends to, because the append may move that sequence's storage (verification: the
  section is present and names the rule and its gate).
- [x] 4.2 Note the new check in the `scripts/` row of `AGENTS.md` (verification: the row names the
  script beside `cppcheck.sh`). Spec: *The optimizer compiles without a possible-uninitialized-use
  diagnostic*.
- [x] 4.3 Close the `GCC 16 reports a possible uninitialized use in Transformation.cpp` entry in
  `TODO.md` the way the other changes do: annotate it with the closing change and what was found, so
  the entry can be removed when the change is archived (verification: the entry carries the
  annotation and no other entry was edited). Base note: the annotated entry itself was recorded by
  the still-open `map-data-memory-budget` change, so a branch cut from `master` carries only the
  pre-existing-warning group this change found; the annotation lands on the entry once that change is
  part of the branch's base.

## 5. Integration verification

- [x] 5.1 Build both configurations without errors: `cmake --build build` and
  `meson compile -C build-meson` (verification: `cmake --build build` exited 0 with no compiler
diagnostic for the touched files and only the pre-existing warnings; `meson compile -C build-meson`
exited 0 with nothing left to rebuild. The two undocumented pre-existing warnings found in the run
are recorded in `TODO.md`).
- [x] 5.2 Run the existing suites of both build systems and confirm no regression:
  `cd build && ctest -j 2 --output-on-failure` and
  `meson test --timeout-multiplier 2 -C build-meson --print-errorlogs` (verification: `ctest` reported
  100% of 140 tests passed with `TransPolygonTest` and the new `VectorSelfAppendTest` among them, and
  `meson test` reported 140 ok, 0 fail with `Check vector self append` among them; `QT_QPA_PLATFORM`
  was set to `offscreen` for the Qt tests).
- [x] 5.3 Check static analysis and formatting on the touched files:
  `clang-tidy -p build` and uncrustify in check mode for the changed files (verification: uncrustify
  wants no change on any added line of the five changed files plus `Tests/src/TransPolygonTest.cpp`
  (its four remaining hunks are pre-existing drift of that file), and clang-tidy reports on the added
  lines only classes this tree already produces - `static` file-local helpers where an anonymous
  namespace would introduce the namespace-indentation drift recorded in the entry about uncrustify,
  magic numbers and pointer arithmetic that the neighbouring cases of the same file produce as well,
  and one possibly-unsafe index read in `MapPainterOpenGL.cpp` in a file full of them. No new
  finding class is attributable to the change).
