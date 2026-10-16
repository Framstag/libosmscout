# Proposal

## Why

Root cause: `TypeSetBytes` (`libosmscout-map/src/osmscoutmap/StyleConfig.cpp:530`) computes the bytes the
per-level type sets retain from a formula that repeats the pre-bitmap representation of a set - one
`TypeInfoRef` (16 bytes) per defined type, per set - instead of asking the sets what they hold.

Evidence:    `Tests/src/StyleConfigLookupCostTest.cpp` case "Defined but unreferenced types add no build work"
fails on HEAD - `REQUIRE( smallDiagnostics.typeSetBytes==largeDiagnostics.typeSetBytes )`, expansion
`3024 (0xbd0) == 14224 (0x3790)`, messages "small type set: slots 30, evaluations 4, table bytes 792, type
set bytes 3024" and "large type set: ... type set bytes 14224" and "large type set: ... type set bytes 14224"; test cases: 11 | 10 passed | 1 failed,
assertions: 73 | 72 passed | 1 failed, and the ctest summary of the run, `1 tests failed out of 1`, run at
2026-10-16T11:06:43Z, git HEAD (loop branch).

Repro:       (cd build && QT_QPA_PLATFORM=offscreen ctest -R StyleConfigLookupCost --output-on-failure)

Spec:        style-configuration / Style-configuration build cost is observable; type-set / The bytes a type
set allocates follow the types it contains   Guideline: guidelines/FileFormatVersion.md (the accessor adds no
stored or serialized layout, so no file format version bump is required)

`fix-typeinfoset-bitmap-storage` replaced the slot per defined type of a set with a bit per defined type plus
a slot per contained type, and left the diagnostic of the build cost behind: the same stylesheet against a
type configuration of 10 and of 60 defined types reports 3024 and 14224 bytes, where the two configurations
retain sets of the same size. The figure is a runtime report of a loaded configuration
(`StyleConfig::BuildDiagnostics::typeSetBytes`,
`libosmscout-map/include/osmscoutmap/StyleConfig.h:961`), summed over the per-level sets of the node, way,
area and route families (`StyleConfig.cpp:778`, `:882`, `:1003`, `:1052`), and it stays a measurement of the
sets' storage only if it reads that storage rather than the representation.

## What Changes

- A set of types reports the bytes it allocates: `TypeInfoSet::AllocatedBytes()` returns the storage of its
  bitmap and of its contained type slots. (`Size()` cannot: it is the number of contained types.)
- The build diagnostic of a style configuration sums `sizeof(TypeInfoSet)+set.AllocatedBytes()` over the
  per-level type sets, so the reported figure follows the sets instead of a reference per defined type.
- The reported number of the shipped stylesheet and type configuration drops accordingly; the measurement
  case keeps reporting it and the scaling case now asserts that two configurations whose sets hold the same
  types report the same figure.
- No call site and no other public signature changes: the accessor is added, every existing method of a set
  and of the diagnostic keeps its signature and its result.

## Capabilities

### New Capabilities

- none.

### Modified Capabilities

- `style-configuration`: the requirement "Style-configuration build cost is observable" states that the
  reported type-set bytes are the bytes the retained sets hold, and gains the scenario "The reported type-set
  bytes follow the retained sets".
- `type-set`: the requirement "The bytes a type set allocates follow the types it contains" states that a set
  reports those bytes, and gains the scenario "A set reports the bytes it allocates".

## Impact

- `libosmscout/include/osmscout/TypeInfoSet.h`: the new accessor with its documentation.
- `libosmscout-map/src/osmscoutmap/StyleConfig.cpp`: `TypeSetBytes` and its comment.
- `libosmscout-map/include/osmscoutmap/StyleConfig.h`: the comment above `typeSetBytes`.
- `Tests/src/StyleConfigLookupCostTest.cpp`: the assertion of the figure and the comment that read the figure
  as an upper bound.
- `Tests/src/TypeInfoSetStorageTest.cpp`: the case that asserts the accessor against the bytes the set
  allocates.
- No other source file changes, no registration in `Tests/CMakeLists.txt` or `Tests/meson.build` (both test
  binaries exist), no database file, type configuration or serialized layout changes, so
  `FILE_FORMAT_VERSION` stays as it is.
- `TODO.md` §104: the entry is removed in phase F, once this change is archived.
