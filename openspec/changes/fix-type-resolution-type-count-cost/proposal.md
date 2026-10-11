# Proposal

## Why

Type resolution during import is meant to cost what an object's tags cost, not what the type definition
contains. Today it does not: the dispatch index that replaced the linear scan still selects candidate
conditions by tag *key* alone, so an object that carries a key used by many types evaluates every
condition declared for that key - even when its value matches none of them. Measured with the code of
the key-level index, a no-match `GetNodeType` grows 1.734 -> 2.696 us and a no-match `GetWayAreaType`
3.635 -> 9.550 us when the type definition grows from 671 to 1527 types (1.55x / 2.63x), because the
number of conditions per key grows with the type set (`shop` 377, `building` 683 references). A
no-match object is the common case in a real import, and its cost is dominated by exactly these
conditions.

This contradicts the `import-type-resolution` requirement "Resolution cost independent of type count"
and its scenario "Early-match object unaffected by type growth". It is also the mechanism that keeps
the parked type set - 854 types behind the `PARKED-NEW-TYPE` marker - from being re-enabled: the
parking was decided because the import cost of the larger type count was not established, and the
value-discriminated resolution is what would establish it.

## What Changes

- Resolving an object's type stops costing more for a tag whose value matches nothing the type
  definition declares: such a tag contributes no candidate condition to evaluate.
- Adding types that declare further values for an already-used tag key does not raise the resolution
  cost of an object that carries none of those values.
- Resolution results are unchanged: the first matching type in type-definition order still wins, a way
  and an area type are still resolved in one pass, and an object whose tags match no type still
  resolves to the ignore state.
- The public type resolution API, the type definition file format and the generated database format are
  unchanged; the type definition still loads with identical types, conditions and options.
- The resolution benchmark covers a key with many declared values and reports the cost of a no-match on
  it, so a regression from type growth on one key is visible.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `import-type-resolution`: the requirement "Resolution cost independent of type count" gains the
  contract that, per tag, only the conditions not discriminated by the tag's value may contribute to
  the cost, plus two scenarios (a no-match value on a crowded key, and type growth on one key). The
  requirement "Resolution performance benchmark" gains a scenario requiring a tag set with a crowded
  key and a value no type declares.

## Impact

Affected code:

- `libosmscout/include/osmscout/TypeConfig.h` — the dispatch index members and the
  `TypeConditionEntry` shape; the test-access friend used by `TypeResolutionTest`.
- `libosmscout/src/osmscout/TypeConfig.cpp` — `BuildTypeResolutionIndex` (index construction) and the
  resolution entry points `GetNodeType`, `GetWayAreaType`, `GetRelationType`.
- `libosmscout/include/osmscout/Tag.h`, `libosmscout/src/osmscout/Tag.cpp` — the condition classes
  report which tag values can make them match, next to the existing key reporting
  (`CollectTagKeys`).

Tests and build:

- `Tests/src/TypeResolutionTest.cpp` — index-shape and dispatch-vs-linear-scan cases for the new
  discrimination, and the crowded-key no-match case.
- `Tests/src/TypeResolutionPerformanceTest.cpp` — a crowded-key tag set in the node and way/area
  benchmarks.
- `Tests/CMakeLists.txt`, `Tests/meson.build` — unchanged; both existing targets carry the new cases.

Records:

- `TODO.md` — the entry "Value-level type index (follow-up to the tag-key dispatch index)" is closed by
  this change; the residual (conditions that are not discriminated by a value, e.g. existence or
  numeric conditions on a crowded key) is re-recorded.

Not affected: the resolution of relation types by the `type` tag, the label/type numbering, and the
parked type set itself - re-enabling the parked types stays a separate decision.
