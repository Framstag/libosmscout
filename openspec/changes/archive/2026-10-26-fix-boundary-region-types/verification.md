# Verification

Every claim and number this change cites, with the command that produced it, the UTC timestamp, the exit
code and the evidence file beside this note. The evidence files are excerpts of the runs, not summaries:
they are kept as the tools wrote them.

## The defect, reproduced and fixed

| claim | evidence | command | time (UTC) | exit |
|-------|----------|---------|-----------|------|
| With the set of types reduced to the four legacy names, the two end-to-end cases fail and the shipped-type-definition case fails | `evidence/probe-four-legacy-names.log` — `LocationBoundaryRegionTest.cpp:122` and `:143` `REQUIRE( !chain.empty() )`, `test cases: 14 \| 12 passed \| 2 failed`, `assertions: 315 \| 313 passed \| 2 failed`; `BoundaryTypesTest.cpp:97` `REQUIRE( Contains(typeConfig,types,"boundary_municipality") )`, `test cases: 2 \| 1 passed \| 1 failed` | `ctest -R 'BoundaryTypesTest\|LocationLookupTest' --output-on-failure` in `build/` (probe: the two optional names of `ResolveBoundaryTypes` skipped, marked `TEMP-PROBE`) | 2026-10-26T22:17:03Z | 8 |
| The form the local patch uses (assert on the two new names) aborts an import of a type definition that predates them | `evidence/probe-assert-legacy-typeconfig.log` — `SIGABRT`, `BoundaryTypesTest: ../libosmscout-import/src/osmscoutimport/BoundaryTypes.cpp:66: … Assertion 'boundaryType' failed.`, `1/1 libosmscout:Check the boundary types FAIL   killed by signal 6 SIGABRT` | `meson test -C debug 'Check the boundary types' --print-errorlogs` (probe: `assert(boundaryType)` + `Set` for the two optional names, marked `TEMP-PROBE`) | 2026-10-26T22:17:33Z | 1 |
| With the resolver and the two levels in the fixture, both new cases pass; the target runs 14 cases | `evidence/ctest-targets-verbose.log` — `All tests passed (317 assertions in 14 test cases)` for `LocationLookupTest` (12 before the change) and `All tests passed (24 assertions in 2 test cases)` for `BoundaryTypesTest` | `ctest -R 'LocationLookupTest\|BoundaryTypesTest' -V` in `build/` | see the header line of the file | 0 |

The second probe is the measured reason the resolver resolves the two names optionally instead of
asserting them: the assertion aborts a debug import of every type definition older than `cd273c581`,
and in a release build (`NDEBUG`) the same code reaches `TypeInfoSet::Set`, which asserts and then reads
`type->GetIndex()` on a null type.

## Builds and suites

| claim | evidence | command | time (UTC) | exit |
|-------|----------|---------|-----------|------|
| The CMake build compiles the whole tree | `evidence/ctest-cmake.log` follows the build of the same directory; the build printed no diagnostic for the touched files | `cmake --build build -j 16` | 2026-10-26T22:27:43Z (suite) | 0 |
| The CMake suite passes, including `HeaderCheckTest` and the two new targets | `evidence/ctest-cmake.log` — `100% tests passed out of 164`, `Total Test time (real) = 29.67 sec` | `QT_QPA_PLATFORM=offscreen ctest -j 8 --output-on-failure -E NumberSetPerformanceTest` in `build/` | 2026-10-26T22:27:43Z | 0 |
| The Meson build compiles the tree in `debugoptimized` with assertions enabled | `evidence/meson-compile.log` — `[26/26] Linking target Tests/LocationServiceTest` | `meson compile -C debug -j 16` | 2026-10-26T22:28:31Z (preceding suite) | 0 |
| The Meson suite passes, twice | `evidence/meson-test.log` — `Ok: 165  Fail: 0`; `evidence/meson-test-run2.log` — `Ok: 165  Fail: 0` | `QT_QPA_PLATFORM=offscreen meson test -C debug --timeout-multiplier 2 --print-errorlogs` | 2026-10-26T22:29:30Z and 2026-10-26T22:30:03Z | 0 |

The first full Meson run of this tree (2026-10-26T22:28:31Z) reported one failure,
`Check the area and way id optimization`, which the three runs above do not reproduce. It is the
allocation-count case of that test, a pre-existing flake unrelated to this change; the message is kept in
`evidence/meson-test-flake-excerpt.log` and filed as `TODO.md` §110.

The first full CMake run of the tree (2026-10-26T22:24:20Z, log overwritten by the green run above)
reported `HeaderCheckTest` failing with:

```
File '/home/tim/projects/libosmscout/libosmscout-import/include/osmscoutimport/private/BoundaryTypes.h
in package 'osmscoutimport.private' has forbidden dependency to package 'osmscout.util'
```

The header included `osmscout/util/Progress.h`; it forward-declares `Progress` now and the translation
unit includes the header, which the final green run verifies.

## Conventions

| claim | verification |
|-------|--------------|
| The new files carry no formatting hunk of their own | `uncrustify -q -c .uncrustify -l CPP -f <file>` compared against the file: 0 differing lines for `BoundaryTypes.h`, `BoundaryTypes.cpp`, `BoundaryTypesTest.cpp`, `LocationBoundaryRegionTest.cpp` (the last three were replaced by the tool's own output first) |
| The two changed import files gain no formatting hunk on the added lines | the same comparison reports 244 differing lines for `GenLocationIndex.cpp` and 79 for `GenRelAreaDat.cpp`, none of them on an added line (a grep for `ResolveBoundaryTypes` / `BoundaryTypes.h` in the diff finds nothing); the drift is the pre-existing one `TODO.md` §88 records repo-wide |
| The new private header does not become installed API | it lives in `include/osmscoutimport/private/`, next to `AreaWayIdReferenceRule.h`; neither `HEADER_FILES` of `libosmscout-import/CMakeLists.txt` nor a Meson install list names it |
| The new translation unit is in both build systems | `libosmscout-import/CMakeLists.txt` `SOURCE_FILES`, `libosmscout-import/src/meson.build` `osmscoutimportSrc` |
| The new test targets are in both build systems | `Tests/CMakeLists.txt` (`BoundaryTypesTest` target with `TESTS_TOP_DIR`, `LocationBoundaryRegionTest.cpp` in `LocationLookupTest`), `Tests/meson.build` (the same, `boundaryTypesEnv` and the `LocationServiceTest` source list) |

## Known limitation

The relation-to-area step (`GenRelAreaDat::ResolveMultipolygonMembers`) has no end-to-end case in this
change. The fixture format the location test uses (`*.olt`, `libosmscout-test/src/osmscout-test/PreprocessOLT.cpp`)
generates ways and nodes from a region tree and cannot generate a relation, and the tree holds no OSM
relation fixture for a boundary. What the two steps read is covered by `BoundaryTypesTest`, because both
take the set from the one resolver; the composition path itself stays untested here.
