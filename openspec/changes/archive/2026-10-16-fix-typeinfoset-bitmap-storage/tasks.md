# Tasks

## 1. Red case

- [x] 1.1 Add `Tests/src/TypeInfoSetStorageTest.cpp` with the two storage cases — this is the red case of the
  spec requirement "The bytes a type set allocates follow the types it contains" and of its scenarios "An
  empty set of a large type configuration allocates a bit per defined type" and "A set of a few types of a
  large type configuration allocates per contained type" — and register it in `Tests/CMakeLists.txt` and
  `Tests/meson.build`. Verify: on HEAD the two cases fail with `REQUIRE( bytes<=BitmapBytes(definedTypeCount) )`,
  expansion `24432 (0x5f70) <= 192`, and `REQUIRE( bytes<=BitmapBytes(definedTypeCount)+4*containedTypes.size()*sizeof(osmscout::TypeInfoRef) )`,
  expansion `36688 (0x8f50) <= 384`; tally `test cases: 2 | 2 failed, assertions: 10 | 8 passed | 2 failed`,
  kept in `evidence/red-test.log`.

## 2. The storage of a set of types

- [x] 2.1 In `libosmscout/include/osmscout/TypeInfoSet.h` and `libosmscout/src/osmscout/TypeInfoSet.cpp`,
  replace the slot per defined type with a bit per defined type plus a slot per contained type, in the order
  of design D1, and keep the class invariants of design D2: the containers are carried by every constructor,
  the copy and move operations, `Set`, `Add`, `Remove`, `Intersection`, `Clear` and `Adapt`. Verify: the two
  storage cases of 1.1 pass, and no header user needs a change (`libosmscout`, `libosmscout-map`,
  `libosmscout-import`, `libosmscout-client*` compile without an edit).

## 3. Behaviour cases

- [x] 3.1 Add the case "Iteration of a set yields the contained types in type index order" to
  `Tests/src/TypeInfoSetStorageTest.cpp` — the spec scenario "Iteration yields the contained types in type
  index order". Verify: with `Set(const TypeInfoRef&)` appending the slot instead of inserting it in index
  order (marked `TEMP-REVERT-PROBE`), the case fails on `REQUIRE( iterated==expected )` — `iterated 4 types
  of 4 contained ones`, `test cases: 5 | 3 passed | 2 failed`, the second failure being the model case's
  `REQUIRE( iterated==Order(model) )` at step 6 — and passes with the fix; the mutated run is
  `evidence/probe-iteration-order.log` and the mutation was reverted
  (`grep -c TEMP-REVERT-PROBE libosmscout/src/osmscout/TypeInfoSet.cpp` is 0).
- [x] 3.2 Add the case "The operations of a set agree with a reference model" — the spec scenario "The
  operations of a set agree with a reference model". Verify: after every step the contained types, the size,
  the membership answers, the intersection and the comparison agree with a `std::set<size_t>` model driven by
  the same random operations.
- [x] 3.3 Add the case "A copy of a set keeps its own contents when the original changes" — the spec scenario
  "A copy of a set keeps its own contents when the original changes". Verify: the copy keeps its types and
  compares unequal to the original after the original was cleared and filled differently.

## 4. Discriminating probe

- [x] 4.1 Restore the per-defined-type allocation of HEAD for the probe: mark the mutation with
  `TEMP-REVERT-PROBE` in `TypeInfoSet.cpp` (or `TypeInfoSet.h`), run `./build/Tests/TypeInfoSetStorageTest`,
  and require the two storage cases of 1.1 to fail again while the behaviour cases of 3.1-3.3 keep passing.
  The probe run kept the bitmap of this change and added a slot per defined type back, which is what its
  numbers show: 24 624 B for the empty set is 24 432 B (one `TypeInfoRef` per defined type) plus the 192 B
  bitmap, and 36 968 B for the set of three. Verify: the quoted failure text is recorded in
  `evidence/probe.log`, the mutation is reverted, and the storage cases pass again.

## 5. Build, gate and regression

- [x] 5.1 Build without new warnings: `cmake --build build -j "$(nproc)"`. Verify: both touched files compile
  and the output contains no warning HEAD does not already produce.
- [x] 5.2 Forced gate, CMake: `cmake --build build -j "$(nproc)"` then
  `cd build && QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR="$PWD/../Tests" TESTS_TMP_DIR="$PWD/Tests" ctest -j 8 -E 'NumberSetPerformanceTest' --output-on-failure`.
  Verify: every test passes; the exclusion is named because `NumberSetPerformanceTest` measures a number set
  of the painter, not the type set this change moves, and run alone it takes 15.70 s
  (`ctest -R NumberSetPerformanceTest`, `evidence/cmake-numberset-perf.log`) against the 19.15 s of the 158
  tests that remain. Copy the run log into `evidence/` before citing it.
- [x] 5.3 Forced gate, Meson: `meson compile -C debug` then
  `cd debug && meson test --timeout-multiplier 2 --print-errorlogs`. Verify: every test passes, including
  "Check type set storage". Copy the run log into `evidence/` before citing it.
- [x] 5.4 Verify no other expectation of the suite moves with the change:
  `grep -rn 'TypeInfoSet' Tests/ libosmscout*/src libosmscout*/include`. Verify: the grep lists the callers
  that copy or pass a set, and none of them asserts a byte count or a slot count of a set.
- [x] 5.5 Check the touched files against the repository conventions: `uncrustify -q -c .uncrustify -l CPP -f
  <file>` for `TypeInfoSet.h`, `TypeInfoSet.cpp`, `TypeInfoSetStorageTest.cpp` and the comment line this
  change rewrites in `StyleConfigLookupCostTest.cpp`, the findings of this revision and of HEAD side by side
  in `evidence/uncrustify.log`. Verify: `TypeInfoSet.h` carries the same 8 finding lines as HEAD (untouched
  iterator, `using` alignment and move constructor), `TypeInfoSet.cpp` 2 lines where this change writes
  against 2 other lines at HEAD, the new `TypeInfoSetStorageTest.cpp` 85 lines, `StyleConfigLookupCostTest.cpp`
  none. Every finding belongs to the two classes uncrustify 0.83 and this tree disagree on - alignment padding
  and the reference spelling `TypeInfoRef & type` (`guidelines/CodeStyles.md`, "Pointers & References"
  mandates the left-attached form `const std::string& name`), which `scripts/format-check.sh check` reports for
  820 of the 943 tracked sources, `TypeInfoSet.h` and `TypeInfoSet.cpp` among them at HEAD as well; the files
  are therefore not reshaped into the config's output.

## 6. Bookkeeping

- [x] 6.1 Mark `TODO.md` §20 `fixed-by fix-typeinfoset-bitmap-storage` in the form the file already uses,
  naming the change so that a later archiving removes the entry. Verify: the entry keeps its `§id`, its
  category and its class, and only its `status` changes.
- [x] 6.2 Verify no guideline is superseded: `guidelines/FileFormatVersion.md` is not triggered, because no
  type configuration, database file format or serialized layout changes. Verify: `git diff --stat` lists
  `libosmscout/include/osmscout/TypeInfoSet.h`, `libosmscout/src/osmscout/TypeInfoSet.cpp`,
  `Tests/CMakeLists.txt`, `Tests/meson.build`, `Tests/src/StyleConfigLookupCostTest.cpp` and `TODO.md`, and no
  `.dat`, `.idx`, `stylesheets/` or generated file; the new `Tests/src/TypeInfoSetStorageTest.cpp` is
  untracked and is therefore listed by `git status --short` (`?? Tests/src/TypeInfoSetStorageTest.cpp`), not
  by `git diff --stat`.
- [x] 6.3 Collect the evidence for the review: `evidence/red-test.log` (HEAD), `evidence/probe.log` (the
  storage probe of 4.1 and of `verification.md`), `evidence/probe-iteration-order.log` (the append probe of
  3.1), `evidence/gate-cmake-build.log`, `evidence/gate-cmake-ctest.log`, `evidence/gate-meson-build.log`,
  `evidence/gate-meson-test.log`, `evidence/cmake-numberset-perf.log` (the excluded test, alone),
  `evidence/post-fix-test.log` (the 192 B and 408 B lines and the five-case tally),
  `evidence/style-config-diagnostic.log` (the `typeSetBytes` figure of `StyleConfigLookupCostTest.cpp:376`)
  and `evidence/uncrustify.log`. Verify: every number, tally and timestamp `verification.md` quotes is
  readable in the file it names, and each of those files names the command it came from.

- [x] 6.4 Record the `style-configuration` requirement this change moves, and refresh the comment this change
  leaves stale: `proposal.md` under "Modified Capabilities" quotes the requirement "Retained memory of a
  loaded style configuration follows the referenced styles", states that the delta of this change is the set
  of types and that no delta is written for the capability, and names the one figure of it this change moves
  (`StyleConfig::BuildDiagnostics::typeSetBytes`, an upper bound now, filed as §104).
  `Tests/src/StyleConfigLookupCostTest.cpp:413`-`:415` no longer claims the per-level type sets "stay sized by
  the defined type count until the set representation is changed (a separate change)". Verify:
  `./build/Tests/StyleConfigLookupCostTest -s "The build diagnostic reports what a style configuration
  prepared"` still passes with `REQUIRE( diagnostics.typeSetBytes>0 )` = `3024 (0xbd0) > 0`,
  `evidence/style-config-diagnostic.log`, and the `grep` of 5.4 finds no expectation of the suite that
  asserts the estimate's exactness.
