# Proposal

## Why

Root cause: `TypeInfoSet` stores one `TypeInfoRef` slot for every type the type configuration defines, in a
container sized to the type count, so every set, every copy of it and every cached tile pays the whole type
count even when the set contains a handful of types, and the set operations and iteration walk that per-type
array.

Evidence:    `Tests/src/TypeInfoSetStorageTest.cpp` case "An empty set of a 1527-type configuration allocates
a bit per defined type" fails on HEAD — `REQUIRE( bytes<=BitmapBytes(definedTypeCount) )`, expansion
`24432 (0x5f70) <= 192`, message "bytes allocated for an empty set of 1527 defined types: 24432"; case "A set
of a 1527-type configuration allocates per contained type, not per defined type" fails —
`REQUIRE( bytes<=BitmapBytes(definedTypeCount)+4*containedTypes.size()*sizeof(osmscout::TypeInfoRef) )`,
expansion `36688 (0x8f50) <= 384`, message "bytes allocated for a set of 3 types in 1527 defined types:
36688"; test cases: 2 | 2 failed, assertions: 10 | 8 passed | 2 failed, run at 2026-10-16T04:50:45Z, git
HEAD `a79e239c9`.

Repro:       ./build/Tests/TypeInfoSetStorageTest
Spec:        type-set / The bytes a type set allocates follow the types it contains   Guideline:
guidelines/FileFormatVersion.md (the set is in-memory only, so no file format version bump is required)

The backlog entry §20 quotes 8 bytes per defined type, i.e. 12.2 KB at 1527 types. Measured today the empty
set allocates 24 432 bytes at 1527 types, twice that, because a type reference is 16 bytes wide. The slot
per defined type is `std::vector<TypeInfoRef> types` (`libosmscout/include/osmscout/TypeInfoSet.h:112`).

The set is copied several times per tile load (`libosmscout-map/src/osmscoutmap/MapService.cpp:499-501`,
`:596-598`, `:668-670`, `:766-768`), retained per cached tile
(`libosmscout-map/include/osmscoutmap/DataTileCache.h:57`, returned by value at `:212`), and its operations
`Intersection`, `Intersects`, `operator==` and `operator!=` (`libosmscout/src/osmscout/TypeInfoSet.cpp`) are
linear in the number of defined types.

## What Changes

- The storage a set of types allocates follows the types the set contains instead of the number of types the
  type configuration defines: a set that holds a few types of a large type configuration no longer allocates
  a slot per defined type.
- The membership test, the iteration order, the set operations (union, difference, intersection, comparison)
  and the reported size keep the results they have today.
- No call site changes: the interface of a set of types is unchanged, so the copies per tile load, the cached
  sets and the stylesheet loading code keep compiling and behaving as before.

## Capabilities

### New Capabilities

- `type-set`: what a set of types is - which types it contains, the bytes it allocates for that, the order in
  which iteration yields them, and the result of the set operations.

### Modified Capabilities

- none. The delta of this change is the set of types; `style-configuration` is not modified. The
  retained-memory requirement of that capability (`openspec/specs/style-configuration/spec.md`) reads: "The
  memory a loaded style configuration retains for style resolution SHALL be governed by the referenced types
  and their levels, not by the number of types the type configuration defines." This change reduces one of the
  structures that requirement governs - the per-level type sets held one type reference per defined type and
  now hold one bit per defined type plus a reference per contained type - but it adds no requirement of that
  capability and changes no requirement of it. It also does not deliver that requirement's scenario "Growth
  of the defined type set does not grow the retained structures" in full: a set still carries a bitmap over
  the defined types, and the lookup position arrays of a loaded configuration are unchanged. No scenario of
  the requirement names a byte figure of a set, so its text, its scenarios and their strength stay as they
  are, and no delta is written for the capability. The one style-configuration figure this change moves is
  the diagnostic estimate `StyleConfig::BuildDiagnostics::typeSetBytes`
  (`libosmscout-map/src/osmscoutmap/StyleConfig.cpp:530`), which the requirement does not name and which
  becomes an upper bound; it stays as it is and is filed as `TODO.md` §104.

## Impact

- `libosmscout/include/osmscout/TypeInfoSet.h`, `libosmscout/src/osmscout/TypeInfoSet.cpp`: the storage
  representation of a set of types and the algorithms of its operations.
- `Tests/src/TypeInfoSetStorageTest.cpp` plus its registration in `Tests/CMakeLists.txt` and
  `Tests/meson.build`.
- `Tests/src/StyleConfigLookupCostTest.cpp`: the comment above the reported type-set figure, which named the
  storage of a set as the reason the figure is reported rather than asserted.
- No public API change and no header user is affected: every constructor, method and iterator of the class
  keeps its signature, so the callers in `libosmscout-map`, `libosmscout-import`, `libosmscout-client*` and
  the tests do not change.
- No database file or type definition layout is touched, so `FILE_FORMAT_VERSION` stays as it is.
- `TODO.md` §20 is marked `fixed-by fix-typeinfoset-bitmap-storage`.
