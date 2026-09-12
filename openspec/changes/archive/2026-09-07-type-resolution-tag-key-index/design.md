## Context

Type resolution during import (`TypeConfig::GetNodeType`, `GetWayAreaType`, `GetRelationType` in `libosmscout/src/osmscout/TypeConfig.cpp:1026-1096`) is a linear scan: for every imported object, every type is visited and its tag conditions evaluated until a match. With 671 types this was acceptable; with 1527 types the measured impact is +47% Preprocess time (16 threads, PBF), worst for ways (500k ways: 2.12s -> 4.41s). The cost scales linearly with type count. See proposal.md - Why for the motivation and spec.md for the required behavior.

Key measured facts that shape the design:

- Only **39 primary tag keys** discriminate all 1489 types; 1078/1489 conditions are simple `"key"=="value"`.
- The condition set is highly skewed (shop 377, amenity 215, building 683 refs, historic 101, office 100).
- Early matches (`highway=residential`) show **zero** slowdown from type growth — the match position is unchanged. The cost hits late-match and no-match objects.
- Ways are the biggest victim: `GetWayAreaType` scans all 536 -> 1289 way/area-capable types.
- Condition trees are built only from five node types (`Tag.h`): AND/OR (`TagBoolCondition`), NOT (`TagNotCondition`), `EXISTS` (`TagExistsCondition`), binary compare (`TagBinaryCondition`), `IN` (`TagIsInCondition`). Every leaf references exactly one tag.

## Goals / Non-Goals

**Goals:**
- No-match and late-match resolution cost proportional to the object's tags, not the type count.
- Exact preservation of resolution semantics (first match in type-definition order; way+area resolved in one pass).
- Measurable improvement via the benchmark added in this exploration (`Tests/src/TypeResolutionPerformanceTest.cpp`).
- Backward compatible: no change to `.ost` format, database format, or public resolution API.

**Non-Goals:**
- Changing the import pipeline stages or parallelization.
- Geometry-aware way/area split (skip area resolution for open ways) — deferrable follow-up, independent of this change.
- Caching resolution results across objects with identical tag sets — deferrable follow-up.

## Decisions

### Decision 1: Tag-key dispatch index (chosen) vs. bitmask prefilter vs. tag-signature cache

**Chosen: tag-key dispatch index.** In `TypeConfig`, build `std::unordered_map<TagId, std::vector<TypeConditionEntry>>` per geometry kind (node / way-area / relation), where each entry holds the condition, its type, and the type index. At resolution time, iterate the object's tag map (typically 2-5 tags), look up each key, and evaluate only the conditions indexed under those keys.

```
CURRENT                               PROPOSED
=======                               ========
GetNodeType(tagMap):                  GetNodeType(tagMap):
  for type in all types:                for (tagId, value) in tagMap:
    for cond in type.conditions:          for entry in index[tagId]:
      if cond.Evaluate(tagMap):             if entry.cond.Evaluate(tagMap):
        return type                             return entry.type
  return ignore                          return ignore

~1100 condition evals/object           ~10-400 evals/object
```

Resolution flow:

```
+-----------+     tags     +---------------+     candidates     +----------------+
| PBF/OSM   | -----------> | GetNodeType / | -----------------> | sort by type    |
| reader    |              | GetWayAreaType|                    | index, evaluate |
+-----------+              +---------------+                    | in order        |
                                 |                              +----------------+
                                 |  match found                        |
                                 v                                    |
                           type assigned to object <------------------+
```

**Alternative A: condition-tag bitmask prefilter.** Per type, precompute a bitmask of referenced TagIds; per object, compute the present-tag bitmask; keep the linear scan but skip a type with a single O(1) bitmask AND before condition evaluation. Exact order, trivial change (`GetNodeType` loop shape unchanged). Cost: still visits all ~1289 types per object (~2us worst case vs ~0.2us for the index). Simpler, slower.

**Alternative B: tag-signature resolution cache.** Cache the resolved type keyed by the canonical (sorted) tag set. OSM data is highly repetitive, so this would help. Cost: canonical key construction per object (sort + hash), cache memory and eviction policy, and it does not help unique objects (the worst case). Complements but does not replace the index.

Rationale: the dispatch index attacks the measured bottleneck directly (condition evaluation count), stays O(tags on object), and the 39-key distribution makes it small and cheap to build. The bitmask is the fallback if order preservation proves fragile; the cache is an orthogonal follow-up.

### Decision 2: Primary-key extraction — post-load tree walk (chosen) vs. parse-time capture

For the index to prune correctly, each condition is indexed under its *primary* keys: the discriminator(s) that must be present for the condition to match. Verified extraction rules:

- AND tree → first child's key (`"landuse"=="farmyard" AND EXISTS "building"` → `landuse`)
- OR tree → **each** branch's first key (33 multi-key OR conditions exist, e.g. `"waterway"=="riverbank" OR ("natural"=="water" AND "water"=="river")` → `waterway` + `natural`)
- NOT → inner key, plus condition goes to a fallback list
- Leaf → its key

Only 3 types have `building` as primary key; the ~680 building-referencing conditions are secondary (keyed on landuse/leisure/amenity etc.). A `building=yes`-only way must evaluate only 3 conditions, not 1289.

**Chosen: post-load tree walk.** Add `virtual void CollectPrimaryKeys(std::vector<TagId>&, bool& hasNot) const` (or equivalent) to `TagCondition` (`libosmscout/include/osmscout/Tag.h`), implemented per subclass. `TypeConfig::RegisterType` or a finalize step walks each type's condition tree and fills the index.

**Alternative: parse-time capture** in `libosmscout/src/osmscout/ost/Parser.cpp`. The parser knows tag names as it builds conditions, but it is generated from a coco/R grammar and every `TAG*COND` production would need key plumbing — invasive and easy to miss a production.

Rationale: the tree walk is a single, testable place; the virtual is additive (no existing caller changes).

### Decision 3: Order preservation — collect-and-sort (chosen) vs. in-place tag order

The current code returns the first match in `types` registration order (`.ost` file order). The index iterates the object's tags (unordered map — arbitrary order), so cross-key ordering differs unless handled.

**Chosen: lazy multiway merge of pre-sorted entry streams.** Each key's entry list and each fallback list is kept sorted by (typeIndex, conditionIndex) at build time (registration order makes this free). At resolution, the object's tag-keyed lists plus the fallback list are merged by repeatedly picking the stream head with the smallest (typeIndex, conditionIndex), deduplicating equal entries across streams, and evaluating in that order until the first match. This preserves the exact evaluation order of the linear scan without a per-object sort of the candidate set (a full collect-and-sort would have regressed early-match objects ~5x). Handles the `"building"=="garage"` vs. generic `EXISTS "building"` overlap correctly.

**Alternative: collect candidates, sort by type index, evaluate in order.** Simpler to reason about, but the per-object sort cost (tens to hundreds of candidates) regresses the common early-match case.

**Alternative: evaluate in tag-iteration order.** Faster, but two types matching under different keys could resolve differently depending on hash ordering — a silent behavior change.

Rationale: correctness first; the sort is cheap at the candidate-set sizes involved.

### Decision 4: Fallback list for NOT-containing conditions

Zero pure-NOT conditions exist in the current stylesheets (every NOT is combined with a positive leaf). But a standalone `!("x" IN [...])` is true when `x` is absent — keying it under `x` would miss that case.

**Chosen: a per-geometry-kind fallback list**, evaluated for every object after the keyed lookup. Empty for current stylesheets; harmless cost; robust against future type definitions.

**Alternative: assume the convention, no fallback.** Faster, but silently wrong if a future stylesheet introduces pure-NOT conditions.

Rationale: correctness robustness for ~0 cost.

### Decision 5: Way/area resolution shape

`GetWayAreaType` resolves way and area types in one pass (one condition evaluation can set both). The index preserves this: one candidate set per object, each candidate's condition evaluated once, setting wayType, areaType, or both per the condition's geometry mask. No API change.

## Risks / Trade-offs

- **Order semantics drift** → The collect-and-sort preserves type-index order; a correctness test compares index resolution against the linear scan for every type's representative tag set plus fuzz inputs; the spec's "generated database files identical" scenario is the end-to-end gate.
- **Primary-key extraction misses a condition shape** → The `CollectPrimaryKeys` walk is unit-tested against the real `map.ost` (all 1489 types loaded, keys extracted, no type left unindexed beyond the fallback). Any missed shape surfaces as a resolution mismatch in the correctness test.
- **API growth (virtual on `TagCondition`)** → Additive; no existing subclass or caller breaks.
- **Memory** → The index is 39 keys x conditions-per-key of lightweight entries; negligible vs. the TypeInfoSet tables already resident.
- **Performance regression from fallback** → Fallback is empty for current stylesheets; if it grows, it is bounded by the number of NOT-containing conditions, not the type count.

## Migration Plan

1. Implement `CollectTagKeys` + index construction + dispatch in `TypeConfig`. During development, the linear scan was kept reachable behind a temporary `SetUseTypeIndex` toggle for A/B verification; the toggle also drove a `--useTypeIndex` import CLI option.
2. Correctness test: for representative tag sets (every type's own conditions + no-match + fuzz), assert dispatch result == linear scan result.
3. Import a fixed extract with dispatch and with linear; assert generated database files byte-identical (done: 65 files identical).
4. Benchmark before/after with `TypeResolutionPerformanceTest` (671 vs 1527 types).
5. **The index is proven correct and unconditional**: the temporary toggle, the `--useTypeIndex` CLI option and the `ImportParameter` plumbing were removed. The linear scan reference implementation now lives only in `Tests/src/TypeResolutionTest.cpp` (via public API) and is used to verify equivalence in every test run.

Correctness invariants the index relies on:
- Conditions must be attached to a type before `RegisterType` (the OST parser does this; the index is built at registration). No code adds conditions after registration.
- Binary-loaded configs (`LoadFromDataFile`) carry no conditions (`TypeInfo::Read` does not persist them), so the empty index is equivalent to the empty-condition linear scan: both resolve everything to ignore.

Rollback: the change is internal to `TypeConfig`; reverting it restores the linear scan without any format or API migration (the linear-scan behavior is the proven baseline and remains covered by the test reference).

## Open Questions

None that would change the specs or task breakdown. (Geometry-aware split and tag-signature cache are recorded as non-goals / follow-ups.)
