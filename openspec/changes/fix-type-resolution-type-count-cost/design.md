# Design

## Context

See `proposal.md` - Why. The key-level dispatch index today:

- `libosmscout/include/osmscout/TypeConfig.h:1100-1109` - `nodeTypeIndex`, `wayAreaTypeIndex`,
  `relationTypeIndex` (`std::unordered_map<TagId,std::vector<TypeConditionEntry>>`) plus the three
  fallback vectors.
- `libosmscout/include/osmscout/TypeConfig.h:1088-1097` - `TypeConditionEntry{type, condition, types,
  typeIndex, conditionIndex}`, kept sorted by `(typeIndex, conditionIndex)` so the merge reproduces the
  type-definition order.
- `libosmscout/src/osmscout/TypeConfig.cpp:1026-1097` - `BuildTypeResolutionIndex`: for every condition
  it collects the primary keys (`TagCondition::CollectTagKeys`, `Tag.h:72`) and appends the entry under
  each key, or to the fallback when the condition may match without any of its keys (negation).
- `libosmscout/src/osmscout/TypeConfig.cpp:1180-1249` - `GetNodeType`/`GetWayAreaType` build one stream
  per tag key of the object, append the fallback, and `FindFirstMatch` (`:1103-1139`) merges the streams
  by `(typeIndex, conditionIndex)`, advancing all streams past a duplicate and evaluating the head in
  order. `GetRelationType` (`:1251`) does the same over `relationTypeIndex`.

The gap: a stream holds *every* condition appended under a key, so for `amenity=foobar` a no-match
object evaluates all `amenity` conditions (147 -> 217 across the growth) and for `building=foobar` all
683. The condition set of a key grows with the type count, which is the measured growth.

Conditions that can be discriminated by a concrete value, from `libosmscout/include/osmscout/Tag.h`:

- `TagBinaryCondition` with `BinaryOperator::equal` - one `(tag, value)` pair.
- `TagIsInCondition` (`Tag.h:221`) - a set of `(tag, value)` pairs.
- `TagExistsCondition` (`Tag.h:153`), other `TagBinaryCondition` operators, `TagBoolCondition`
  (`Tag.h:118`), `TagNotCondition` (`Tag.h:88`) - no single concrete value; they stay key-level.

## Flow

Resolution of a node after the change, for an object with `amenity=foobar` where no type declares
`foobar` (the tag still builds a stream, but the stream is empty):

```
  GetNodeType(tagMap)
    |
    | for each tagEntry (key,value):
    |     nodeTypeIndex[key]  ->  valueBuckets[value]   (exact conditions for this value)
    |                        +   keyOnly                (conditions not discriminated by a value)
    |     append non-empty stream
    |
    | append nodeFallbackConditions (if any)
    |
    +--> FindFirstMatch(streams):
           no stream holds a candidate  ->  return typeInfoIgnore
           (before: the amenity stream held every amenity condition -> all evaluated)
```

The same shape applies to `GetWayAreaType` over `wayAreaTypeIndex` and to `GetRelationType` over
`relationTypeIndex`.

## Goals / Non-Goals

**Goals:**

- A tag whose value matches no declared condition contributes no candidate to evaluate.
- Adding types with conditions on new values of a used key does not raise the cost of a no-match object.
- Resolution results are bit-identical to the current dispatch index and to the linear scan: first match
  in type-definition order, way/area in one pass, unknown tags ignored.
- The type definition file format, the database format and the public API are untouched.

**Non-Goals:**

- Discriminating conditions that carry no concrete value (existence, numeric comparison, negation).
  They stay in the key-level stream and their cost still grows with the number of such conditions on a
  key; this is re-recorded as a TODO entry, not closed here.
- Re-enabling the parked type set; this change removes the resolution-cost obstacle, it does not decide
  the parking.
- Any change to `CollectTagKeys` semantics used by the existing key index and its tests.
- Caching or memoizing resolution results.

## Decisions

### D1: Where the value discrimination is stored

**Chosen: extend each key index to a per-key value map plus a key-only list.**

The index member becomes a small struct per geometry kind:

```cpp
struct TypeKeyIndex
{
  std::unordered_map<std::string,std::vector<TypeConditionEntry>> valueBuckets; //!< exact conditions by declared value
  std::vector<TypeConditionEntry>                                keyOnly;      //!< conditions not discriminated by a value
};
std::unordered_map<TagId,TypeKeyIndex> nodeTypeIndex; // and wayArea/relation
```

`GetNodeType` reads `valueBuckets[tagEntry.second]` and `keyOnly` for the object's tag value.

Alternatives:

1. *Flat map keyed by `(TagId,std::string)`* - one `unordered_map` with a composite key. Same lookup
   cost, but it cannot express "conditions on this key that are not value-discriminated" without a
   sentinel value, and it loses the cheap "does this key exist at all" check the current code uses.
   Rejected.
2. *Keep `std::vector<TypeConditionEntry>` per key, sorted by declared value, and binary-search the
   value* - no extra hash map per key, but it needs a parallel value array and a comparator, and the
   `keyOnly` entries interleave; more code for no measurable gain at these sizes (largest bucket 683).
   Rejected.
3. *Two-level `unordered_map<TagId,unordered_map<std::string,...>>` without `keyOnly`* - forces a
   second lookup for an empty bucket per tag and has nowhere to put existence/negation conditions;
   rejected.
4. *Bloom filter per key over declared values* - rejects most no-match values with one hash, but a
   positive still evaluates the full key stream and the filter has to be rebuilt with the index; not
   worth the complexity while the value map is exact. Rejected.

### D2: How a condition is classified as value-discriminated

**Chosen: let each condition report a conservative "matches only for these (key,value) pairs" set.**

A new virtual alongside `CollectTagKeys` (same file, same style) reports the pairs a condition can match
for, and whether the report is exhaustive (the condition cannot match unless one of the pairs holds):

- `TagBinaryCondition` with `equal` -> one pair, exhaustive.
- `TagIsInCondition` -> one pair per value, exhaustive.
- `TagExistsCondition`, other operators -> no pairs, not exhaustive.
- `TagBoolCondition(boolAnd)` -> union of the children's exhaustive pairs, exhaustive if at least one
  child is exhaustive (a match must satisfy that child).
- `TagBoolCondition(boolOr)` -> union of all children's pairs, exhaustive only if every child is.
- `TagNotCondition` -> no pairs, not exhaustive.

An entry is appended to `valueBuckets[value]` for each exhaustive pair keyed by one of the condition's
primary keys; keys of the condition without an exhaustive pair, and the whole condition when the report
is not exhaustive, go to `keyOnly`. This is conservative: a condition that is wrongly treated as
non-exhaustive only loses speed, never correctness.

Alternatives:

1. *Special-case only the top-level `TagBinaryCondition`/`TagIsInCondition`* - simplest, but an `OR`
   over two values (common in the type definition) then lands in `keyOnly` although it is exactly
   exhaustive; rejected as it leaves a visible share of the win on the table.
2. *Full constraint propagation with intersection for `AND`* - narrower buckets, but an intersection can
   become empty (then the condition never matches and could be dropped) and needs care with multi-key
   conditions; the union superset is already safe and orders of magnitude smaller than the key stream.
   Rejected as more machinery than the contract needs.
3. *Classify by re-parsing the condition string* - brittle and duplicates the condition classes; rejected.

### D3: How the resolution entry points query the index

**Chosen: keep `FindFirstMatch` and the stream merge; only the stream source changes.**

Each tag of the object appends its `valueBuckets[value]` and its `keyOnly` (and the key-level stream
disappears). Because both are sorted by `(typeIndex, conditionIndex)` at build time and the append order
follows tag order, the merge and the duplicate-advance in `FindFirstMatch` stay valid; a condition that
was appended under two keys of one object is still evaluated once.

Alternatives:

1. *Evaluate the value buckets first, then the key-only/fallback streams* - breaks first-match order
   when a key-only condition of an earlier type competes with an exact condition of a later type;
   rejected.
2. *Concatenate and sort the candidate streams per call* - an allocation and a sort per resolution;
   rejected.
3. *Precompute a merged candidate list per `(key,value)` at build time* - duplicates entries per key
   combination an object can carry; rejected (memory).

### D4: How "unchanged resolution" and "no-match cost" are verified

**Chosen: index-shape assertions plus the existing dispatch-vs-linear-scan equivalence.**

`Tests/src/TypeResolutionTest.cpp` already reaches the private index through the
`TypeResolutionIndexTestAccess` friend (`:31-95`) and compares dispatch results with a linear scan built
from the same type definition (`:246-283`). The new cases:

- a synthetic definition with many values on one key: a no-match value reaches an empty candidate set
  (the exact criterion of the spec scenario), while every declared value reaches its condition;
- a synthetic definition grown by more types on the same key: the candidate set for the no-match value
  stays empty;
- the existing equivalence matrix is extended with the crowded-key definition so correctness is guarded
  by the linear scan, not by the index.

Alternatives:

1. *Timing assertion on `TypeResolutionPerformanceTest`* - the number the spec cares about, but flaky on
   shared CI and load-dependent; rejected as a gate (the benchmark still reports it, task 5).
2. *Runtime counter of evaluated conditions on `TypeConfig`* - deterministic, but adds a diagnostic to
   the public type config for one test; the friend-based index access already exists and asserts the
   same fact one level closer. Rejected.
3. *Only extend the benchmark* - makes the win visible but lets a regression in the buckets pass; the
   benchmark is not an assertion.

## Risks / Trade-offs

- **Memory: every value-discriminated condition is now held once per declared value, not once per key.**
  The total number of entries is unchanged (a condition declares one value, one bucket); the overhead is
  one `std::string` key and one small vector per distinct value, i.e. up to the condition count of the
  largest key (683 for `building`). Acceptable; the type definition is loaded once per process.
- **Build time of the index grows with the number of distinct values.** Same order as today's append
  loop, plus a hash per value; measured against `SymbolsAll --list` type-definition parse in task 6.
- **The introspection is a new virtual on the condition hierarchy.** It is pure virtual next to
  `CollectTagKeys`, so a future condition class cannot forget it silently; the fallback implementations
  are conservative.
- **A condition keyed on several keys with different values** is appended under each `(key,value)` pair
  it declares; the duplicate-advance in `FindFirstMatch` already handles the object carrying both keys.
- **Conditions that are not value-discriminated still scale with the key.** Documented as the residual
  TODO entry; the spec scenario is scoped to declared values, which is what the index can guarantee.
- **The relation index is included although the `type` tag has few values.** Consistency of the three
  indexes is cheaper to reason about than a special case; no measurable cost.

## Migration Plan

No data, format or API change; nothing is re-imported. The change is a resolution-cost fix, so the
rollback is a revert. The measurement record below is produced by `TypeResolutionPerformanceTest`
(`--iterations 200000`) on `stylesheets/map.ost` and on the same file with the `PARKED-NEW-TYPE` definitions
re-enabled (the `// ` prefix of the marked definitions stripped), which is 673 types (337 node / 93 way /
445 area) against 1522 types (854 / 187 / 1097):

| build | type set | node `amenity=foobar` (crowded) | way/area `shop=foobar` (377 conditions) |
|-------|----------|--------------------------------|----------------------------------------|
| before | 673 | 1.14 us/op | 0.59 us/op |
| before | 1522 | 1.39 us/op | 4.24 us/op |
| after | 673 | 0.11 us/op | 0.17 us/op |
| after | 1522 | 0.09 us/op | 0.11 us/op |

Before the change the crowded-key no-match walks every condition of the key and grows with the type count
(way/area `shop=foobar` 0.59 -> 4.24 us/op, 7.2x). After the change both rows are in the same order as the
single-tag rows and no longer grow with the type set. The absolute values vary by up to ~2x between runs on
this machine (the same build measured 0.04-0.11 us/op for the node row in two runs), so the ratio and the
flatness across the two type sets are the result, not the individual numbers. The residual stays: a condition
without a declared value (existence, numeric comparison, negation) is still in the key-level list and still
scales with the number of such conditions on one key - it is recorded in `TODO.md`.

## Open Questions

- Whether the key-only/fallback buckets deserve their own discrimination later (e.g. an interval or
  prefix index for numeric and existence conditions). Deferrable - it does not change this change's
  specs, approach or tasks.
