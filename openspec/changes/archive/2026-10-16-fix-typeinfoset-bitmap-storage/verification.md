# Verification

Iteration §20 of the fix loop, change `fix-typeinfoset-bitmap-storage`.

## Root cause and red case

`TypeInfoSet` held one `TypeInfoRef` slot per type the type configuration defines
(`libosmscout/include/osmscout/TypeInfoSet.h:112` before the change, `std::vector<TypeInfoRef> types` sized to
the type count by the constructor at `libosmscout/src/osmscout/TypeInfoSet.cpp:26`-`:29`), so every set, every copy
of it and every cached tile paid the whole type count.

`Tests/src/TypeInfoSetStorageTest.cpp` reproduces it on HEAD — full run in `evidence/red-test.log` (run at
2026-10-16T04:50:45Z, git HEAD `a79e239c9`):

```
A set of a 1527-type configuration allocates per contained type, not per defined type
  REQUIRE( bytes<=BitmapBytes(definedTypeCount)+4*containedTypes.size()*sizeof(osmscout::TypeInfoRef) )
with expansion:
  36688 (0x8f50) <= 384 (0x180)
  "bytes allocated for a set of 3 types in 1527 defined types: 36688"
```

The two storage cases failed on HEAD with `test cases: 2 | 2 failed` and `assertions: 10 | 8 passed | 2
failed`. Backlog entry: `TODO.md` §20.

The bound of that case is the widened one,
`2*BitmapBytes(definedTypeCount)+2*containedTypes.size()*sizeof(osmscout::TypeInfoRef)`: a set that is filled
by its type indices grows its bitmap and its slot container as it goes, so the bound covers that growth on
top of their final size. `evidence/red-test.log` carries the bound the case had on HEAD
(`BitmapBytes+4*containedTypes.size()*...`, expansion `36688 <= 384`), `evidence/probe.log` the widened one
(`36968 (0x9068) <= 480 (0x1e0)`).

## The storage before and after

Measured with the allocation counter of `Tests/src/TypeInfoSetStorageTest.cpp` (the counter sums the bytes
every heap allocation asks for; the run is `evidence/red-test.log` for HEAD, which fails on the two numbers,
and `evidence/post-fix-test.log` for the change, whose `-s` section carries
`bytes allocated for an empty set of 1527 defined types: 192` and
`bytes allocated for a set of 3 types in 1527 defined types: 408`):

| set | defined types | HEAD | this change | factor |
|---|---|---|---|---|
| empty set, constructed for the type configuration | 1527 | 24 432 B | 192 B (a bit per defined type) | 127 |
| set of 3 types, constructed from them | 1527 | 36 688 B | 408 B (bitmap plus the three types) | 90 |

The change also removes the linear walk over the defined types: `Intersects` and the comparison compare 64
bit words, `Intersection` visits the contained types, and `IsSet` tests one bit.

## Behaviour

`Tests/src/TypeInfoSetStorageTest.cpp` holds five cases; `evidence/post-fix-test.log` records

```
All tests passed (27570 assertions in 5 test cases)
```

- two storage cases - the spec scenarios "An empty set of a large type configuration allocates a bit per
  defined type" and "A set of a few types of a large type configuration allocates per contained type";
- "Iteration of a set yields the contained types in type index order";
- "The operations of a set agree with a reference model" - 200 random operations (`Set`, `Remove`, `Add`,
  `Remove(const TypeInfoSet&)`, `Intersection`, `Clear`) against a `std::set<size_t>` model, comparing the
  contained types, the size, the membership of every defined type, `Intersects` and the comparison after each
  step, plus the two operations that replace the content from a list of types;
- "A copy of a set keeps its own contents when the original changes", including `Adapt` to a larger type
  configuration.

## Discriminating probe

`evidence/probe.log`: with the per-defined-type slot allocation of HEAD laid back on top of the bitmap for
the probe (marked `TEMP-REVERT-PROBE`), exactly the two storage cases fail again

```
REQUIRE( bytes<=BitmapBytes(definedTypeCount) )
with expansion:   24624 (0x6030) <= 192
REQUIRE( bytes<=2*BitmapBytes(definedTypeCount)+2*containedTypes.size()*sizeof(osmscout::TypeInfoRef) )
with expansion:   36968 (0x9068) <= 480 (0x1e0)
test cases:     5 |     3 passed | 2 failed
```

while the three behaviour cases pass, i.e. the two storage cases discriminate the storage and nothing else.
The empty set then asks for
24 624 B, which is the 24 432 B of one slot per defined type that `evidence/red-test.log` measures on HEAD plus
the 192 B bitmap that `evidence/post-fix-test.log` measures for the change: the probe kept the new storage and
added the HEAD allocation next to it.

`evidence/probe-iteration-order.log` is the second probe, the one task 3.1 states: with `Set(const
TypeInfoRef&)` appending the slot instead of inserting it in index order (also marked `TEMP-REVERT-PROBE`), the
case "Iteration of a set yields the contained types in type index order" fails on
`REQUIRE( iterated==expected )` (`iterated 4 types of 4 contained ones`) and the model case on
`REQUIRE( iterated==Order(model) )` at step 6 (`{ 24, 60, 49 } == { 24, 49, 60 }`), while the two storage cases
and the copy case pass (`test cases: 5 | 3 passed | 2 failed`).

Both mutations were reverted;
`grep -c TEMP-REVERT-PROBE libosmscout/src/osmscout/TypeInfoSet.cpp` is 0.

## Forced gate

The change touches product code - `libosmscout/src/osmscout/TypeInfoSet.cpp` and the public header
`libosmscout/include/osmscout/TypeInfoSet.h` - so the delta class is `code` and its gate is the
both-build-system one; `.pi/skills/verify-build-and-tests/scripts/gate-class.sh` reports that it has no
committed delta to classify for a working-tree-only change and asks for the classification by hand, which is
the one above. The gate ran after the last write to a source
file (the last source mtimes are 2026-10-16T05:28:20Z for `libosmscout/src/osmscout/TypeInfoSet.cpp`,
`libosmscout/include/osmscout/TypeInfoSet.h` and `Tests/src/TypeInfoSetStorageTest.cpp`, and
2026-10-16T05:19:40Z for `Tests/src/StyleConfigLookupCostTest.cpp`; every build and test run below is later,
so the tallies belong to this revision):

| step | command | result | log |
|---|---|---|---|
| build | `cmake --build build -j "$(nproc)"` | 544 targets, exit 0 at 2026-10-16T05:32:50Z, `TypeInfoSet.cpp` and the new test recompiled and linked, no warning or error of theirs; the log's 64 warnings are the 13 doxygen tag warnings of the documentation step and 51 compiler warnings of unrelated files (Qt deprecations, an unused variable, an ignored `nodiscard`) that the header change recompiles | `evidence/gate-cmake-build.log` |
| suite | `cd build && QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR="$PWD/../Tests" TESTS_TMP_DIR="$PWD/Tests" ctest -j 8 -E 'NumberSetPerformanceTest' --output-on-failure` | `100% tests passed out of 158`, 19.15 s, exit 0 at 2026-10-16T05:33:44Z | `evidence/gate-cmake-ctest.log` |
| build | `meson compile -C debug` | the log ends at `[365/365]`, exit 0 at 2026-10-16T05:41:18Z, `TypeInfoSet.cpp` and the new test recompiled, the library and the test executables relinked | `evidence/gate-meson-build.log` |
| suite | `cd debug && meson test --timeout-multiplier 2 --print-errorlogs` | `Ok: 159`, `Fail: 0`, exit 0 at 2026-10-16T05:41:44Z | `evidence/gate-meson-test.log` |

Each of the four logs carries its command, its UTC timestamp, its exit code and the sha256 of the revision it
ran against, so the tallies are pinned to the tree they belong to:

```
889ee70a02090436c304a9d053d9448bf25651374c0605e6305957f657c0526a  libosmscout/include/osmscout/TypeInfoSet.h
c1ef5d81939e2de1a91f04626ac77d9ad603b1654ad73eadffe6a815980d5070  libosmscout/src/osmscout/TypeInfoSet.cpp
f36a85c48d6a907c7b874f0e75e48d307375a54fa916e8d2366a825740e529e1  Tests/src/TypeInfoSetStorageTest.cpp
a9edb36256bd8dd3a3cc2ceee8ae688a1e8c42e182e16dabd1420c26de1225fd  Tests/src/StyleConfigLookupCostTest.cpp
```

The public header is reached by the other targets, so the gates rebuilt the dependents of it; `Check type
set storage` and the CMake `TypeInfoSetStorageTest` both run the revision above.

`NumberSetPerformanceTest` is excluded from the CMake suite: it measures the painter's number set, not the
type set this change moves. Run alone it takes 15.70 s (`ctest -R NumberSetPerformanceTest`, exit 0 at
2026-10-16T05:42:33Z, `evidence/cmake-numberset-perf.log`) against the 19.15 s of the 158 tests that remain in
the suite. The Meson run carries no exclusion and reports it as "Check number set performance". The new test
runs in both systems (`TypeInfoSetStorageTest` in the CMake log, "Check type set storage" in the Meson log).

`git grep` found no other expectation of the suite that moves with the change: no test asserts a byte or slot
count of a set, and `Tests/src/StyleConfigLookupCostTest.cpp:411` asserts `tableBytes` only, with the comment
at `:413`-`:415` and the `INFO` at `:416` reporting the per-level type sets instead of asserting them.

## Files and conventions

`git diff --stat` lists `libosmscout/include/osmscout/TypeInfoSet.h`,
`libosmscout/src/osmscout/TypeInfoSet.cpp`, `Tests/CMakeLists.txt`, `Tests/meson.build`,
`Tests/src/StyleConfigLookupCostTest.cpp` (the comment at `:413`-`:415` only) and `TODO.md`; the new
`Tests/src/TypeInfoSetStorageTest.cpp` is untracked, so `git status --short` carries it
(`?? Tests/src/TypeInfoSetStorageTest.cpp`) and `git diff --stat` does not. No `.dat`, `.idx`, `stylesheets/`
or generated file is touched, and `guidelines/FileFormatVersion.md` is not triggered: the set is in-memory
only and no type configuration or database file layout changes.

Uncrustify (`uncrustify -q -c .uncrustify -l CPP -f <file>`, 0.83.0, the lines it would change per file in
`evidence/uncrustify.log`):

- `libosmscout/include/osmscout/TypeInfoSet.h`: 8 lines now and the same 8 lines at HEAD - the iterator's
  `using`-alignment, two continuations and the move constructor, none of them lines this change writes.
- `libosmscout/src/osmscout/TypeInfoSet.cpp`: 3 lines now (the license block's first line, which uncrustify
  would de-indent as it does at HEAD, plus `const TypeInfoRef& type=contained[i];` in `Intersection` and the
  alignment of the declaration after it) against 2 other lines at HEAD (that license line and a continuation
  in `Intersects`); uncrustify 0.83 wants the reference spelled `const TypeInfoRef & type` and the
  declarations padded, which `guidelines/CodeStyles.md`, "Pointers & References" mandates left-attached
  (`const std::string& name`) and which `TODO.md` §8 tracks for the same class in `TypeConfig.cpp`
  (`const FeatureValue * otherValue`).
- `Tests/src/TypeInfoSetStorageTest.cpp`: 85 lines, all of the same two classes (reference and pointer
  spacing, alignment padding), the file is new.
- `Tests/src/StyleConfigLookupCostTest.cpp`: none now and none at HEAD.

So "no drift" would be false: three of the four touched C++ files differ from uncrustify's output in the
classes above, and `scripts/format-check.sh check` reports 820 of the 943 tracked sources unformatted,
`TypeInfoSet.h` and `TypeInfoSet.cpp` among them at HEAD too. The files are not reshaped into the config's
output, because that
output contradicts the reference and pointer placement the guidelines mandate.

The first full build failed with the member name `slots` (`libosmscout/include/osmscout/TypeInfoSet.h:117`
then), because Qt defines `slots` as a macro in every translation unit that includes a Qt header
(`libosmscout-map-qt/src/osmscoutmapqt/MapPainterQt.cpp`); the member is named `contained`. The failed build
is kept as `evidence/first-build-qt-slots-collision.log`.

## Left out deliberately

`StyleConfig::BuildDiagnostics::typeSetBytes` is still computed by `TypeSetBytes`
(`libosmscout-map/src/osmscoutmap/StyleConfig.cpp:530`) as
`typeSets.size()*(sizeof(TypeInfoSet)+typeCount*sizeof(TypeInfoRef))`, i.e. from the storage a set had
before this change. The number it reports is an upper bound now. It is a diagnostic of the *style
configuration*, and the delta of this change is the set and its operations, so it stays as it is and is filed
as `TODO.md` §104; the comment above the figure it feeds was refreshed (`Tests/src/StyleConfigLookupCostTest.cpp:413`-`:415`),
and `proposal.md` under "Modified Capabilities" quotes the requirement of that capability this change moves
and states why no delta is written for it. The assertion on the same figure,
`Tests/src/StyleConfigLookupCostTest.cpp:376` `REQUIRE( diagnostics.typeSetBytes>0 )`, still holds and still
means what the case name says - the diagnostic reports a figure - because the formula it comes from is
unchanged; it reports `3024 (0xbd0) > 0` in `evidence/style-config-diagnostic.log`, and no test asserts the
estimate's exactness.
