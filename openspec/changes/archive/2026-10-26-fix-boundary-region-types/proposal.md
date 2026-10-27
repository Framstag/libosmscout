# Proposal

## Why

`stylesheets/map.ost` declares five administrative boundary types — `boundary_country` (2),
`boundary_state` (4), `boundary_county` (6), `boundary_municipality` (8), `boundary_suburb` (10) — and
the generic `boundary_administrative` fallback after them. Type resolution takes the first declared
matching type (`TypeConfig::FindFirstMatch`, `libosmscout/src/osmscout/TypeConfig.cpp:1194`, where
`PickNextHead` `:1227` prefers the lowest `typeIndex`), so a level-8 boundary resolves to
`boundary_municipality` and a level-10 one to `boundary_suburb`; before commit `cd273c581` of the
stylesheet (2026-08-12) neither type existed, and both levels fell through to the generic
`boundary_administrative`.

The two steps of the import pipeline that decide what a boundary is still enumerate the four legacy
names and nothing else:

- `libosmscout-import/src/osmscoutimport/GenLocationIndex.cpp:2761-2783` fills the `boundaryTypes` set
  `GetBoundaryAreas` scans with. Any area whose type is not in that set is skipped at `:1245`, above
  all three `errorReporter->ReportLocation(...)` calls (`:1252`, `:1261`, `:1266`), so a level-8 or
  level-10 boundary is dropped without a line in any import report.
- `libosmscout-import/src/osmscoutimport/GenRelAreaDat.cpp:651-673` fills the set
  `ResolveMultipolygonMembers` uses to decide `ComposeBoundaryMembers` (`:697`) over
  `ComposeAreaMembers`; a nested boundary relation member of either type takes the generic path and
  is otherwise dropped with `Warning: Unsupported relation (...) reference` (`:717`).

The result is a database whose region index stops at the county: the reported symptom is a search row
naming `Kreis Unna` instead of `Bergkamen` for the query `Rosenweg 20 Bergkamen`, and no perfect-match
marking, because the town never enters the region chain of an entry (Framstag/NaviVeylin#677). Every
map built from the shipped stylesheet since that commit is affected, and a map built from an older
type definition is not — the defect is a regression in the pipeline's enumeration of boundary types,
not in the stylesheet.

## What Changes

- The import pipeline treats the types the type definition declares as administrative boundaries as
  boundaries, for both the location index and the relation-to-area composition: a level-8 boundary
  becomes a region of the location index and a level-10 boundary likewise, nested under the county
  and the municipality respectively.
- The boundary type list is resolved in one place instead of being enumerated separately by every step
  that needs it, so the next type the stylesheet splits out of the generic rule is not dropped by a
  step that was not updated.
- A type definition that predates `boundary_municipality` and `boundary_suburb` keeps importing: a
  type the pipeline treats as a boundary but that a type definition does not declare is skipped with a
  diagnostic instead of aborting the run (debug) or dereferencing a null type (release). The four
  legacy names are unchanged in this respect; a type definition that lacks them already aborts today.
- The `admin_level` limit stays as it is: `ImportParameter` defaults to level 10
  (`libosmscout-import/src/osmscoutimport/ImportParameter.cpp:104`), so both new levels are in range of
  a default import and need no parameter change.

Not changed: the type definitions and the stylesheets (which already carry the types), the layout of
`location.idx` and of `areas.dat`, the region lookup and search API, and the `location.html` /
`relation.html` reports.

## Capabilities

### New Capabilities

- `import-administrative-boundaries`: what the import pipeline treats as an administrative boundary —
  every boundary type the type definition declares — and what a boundary becomes in the location index,
  including the case of a type definition that does not declare a type the pipeline knows.

### Modified Capabilities

None. `location-index-lookup` states what a lookup on the index costs, `import-type-resolution` how a
way's and an area's type is resolved from its tags; neither states which resolved type becomes a region.

## Impact

Affected code:

- `libosmscout-import/include/osmscoutimport/private/BoundaryTypes.h` (new) and
  `libosmscout-import/src/osmscoutimport/BoundaryTypes.cpp` (new) — the one resolver, its optional
  handling of the types a type definition may lack and its diagnostic.
- `libosmscout-import/src/osmscoutimport/GenLocationIndex.cpp` — the boundary type set of `Import()` is
  taken from the resolver (replacing the six `GetTypeInfo` calls at `:2761-2783`).
- `libosmscout-import/src/osmscoutimport/GenRelAreaDat.cpp` — the boundary type set of
  `ResolveMultipolygonMembers` is taken from the resolver (replacing the six calls at `:651-673`).
- `libosmscout-import/CMakeLists.txt`, `libosmscout-import/src/meson.build` — the new translation unit
  in both build systems.

Tests and build:

- `Tests/src/BoundaryTypesTest.cpp` (new) — the resolver against the shipped `stylesheets/map.ost` and
  against a type definition that declares only the four legacy types; a new `BoundaryTypesTest` target
  in `Tests/CMakeLists.txt` and `Tests/meson.build`.
- `Tests/LocationTest.olt` — a level-8 boundary and a nested level-10 boundary under `Kreis Unna`, so
  the fixture of the location test carries the two levels the shipped type definition declares.
- `Tests/src/LocationBoundaryRegionTest.cpp` (new) — the two levels as regions of the imported
  database, nested as the fixture nests them, in the existing `LocationLookupTest` target
  (`Tests/CMakeLists.txt:92`, `Tests/meson.build:470`).

Known limitation, recorded in `design.md`: the relation-to-area composition is covered by the resolver
unit test only. The location test's fixture format (`*.olt`) generates ways, not relations, so no
end-to-end case drives a nested boundary relation member through `GenRelAreaDat`; the shared resolver
is what keeps the two steps from disagreeing.
