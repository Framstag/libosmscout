# Design

## Context

See `proposal.md` — Why for the motivation; the requirements are in `specs/style-configuration/spec.md`.

Current shape of the code that this design changes (all in `libosmscout-map`):

- Every style family keeps its resolved selectors in a dense table indexed by **type index × magnification
  level**. The tables are declared in `libosmscout-map/include/osmscoutmap/StyleConfig.h:491-539` and held
  as members at `:601-655` (`nodeTextStyleSelectors`, `nodeIconStyleSelectors`, `wayLineStyleSelectors`,
  `wayPathTextStyleSelectors`, `wayPathSymbolStyleSelectors`, `wayPathShieldStyleSelectors`,
  `areaFillStyleSelectors`, `areaBorderStyleSelectors`, `areaTextStyleSelectors`, `areaIconStyleSelectors`,
  `areaBorderTextStyleSelectors`, `areaBorderSymbolStyleSelectors`, `routeLineStyleSelectors`,
  `routePathTextStyleSelectors`).
- `SortInConditionals<S,A>` (`libosmscout-map/src/osmscoutmap/StyleConfig.cpp:544-554`) sizes one such table
  with `selectors.resize(typeConfig.GetTypeCount())` and, per type, `selector.resize(maxLevel+1)`. It is the
  single place where the type-count dependence is created; the level lists are created per type whether or
  not a conditional names that type.
- `SortInConditionalsBySlot` (`:608-632`) runs the same step once per style slot, so the text and line
  families allocate **slots × types × levels** containers.
- Lookup walks the table for the object's type index: `GetFeatureStyle<S,A>` (`:1267-1304`) takes
  `selectors[typeIndex]`, clamps the level to `styleSelectors.size()-1`, and iterates the selectors of that
  level. It starts with `assert(!styleSelectors.empty())` (`:1273`), i.e. the current code assumes every
  defined type has an entry.
- The postprocess step then walks the tables again for the visibility bounds
  (`PostprocessVisibilityBounds`, `:1036-1066`) through `HasStyle` (`:672-684`), which asserts each type's
  entry is non-empty.
- The cost is not only allocation: `SortInConditionals`' inner loop (`:556-571`) tests every conditional
  against **every defined type** (`filter.HasType(type)`), and `CalculateUsedTypes` (`:520-541`) repeats that
  once per magnification level. The walking term therefore scales with the defined type count independently
  of how large the tables are.
- `StyleFilter` (`libosmscout-map/include/osmscoutmap/StyleConfig.h:243-281`) stores `filtersByType` plus a
  dense `TypeInfoSet types` and exposes only `HasType()`. Because `TypeInfoSet` is a full-length
  `std::vector<TypeInfoRef>` (`libosmscout/include/osmscout/TypeInfoSet.h:112`), enumerating a filter's types
  is itself O(defined types): there is no list of the types one rule names, which is what a
  type-count-independent walk needs.
- `nodeTypeSets` / `wayTypeSets` / `areaTypeSets` / `routeTypeSets` (`:653-666`, `:719`, `:826`, `:879`) are
  public members resized per level whose elements are `TypeInfoSet`, i.e. they are type-count sized as well.

Constraint that shapes the whole design: `libosmscout-map/src/osmscoutmap/MapPainter.cpp` is being rewritten
by PR #1784 (`fix-label-flicker`), and `MapService.cpp` / `DataTileCache.h` are live in PR #1860, #1861 and
#1862. The change therefore adapts the accessor implementations inside `StyleConfig` and keeps every
signature as it is; it touches no painter file.

## Goals / Non-Goals

**Goals:**

- The build work and the retained memory of a loaded style configuration follow the types a stylesheet
  references and the levels it uses.
- The walking part of the build follows the types a rule names, so it does not scale with the defined type
  count either — the table size alone is not the requirement.
- Resolution results, level gating and rule composition are bit-for-bit the behaviour of today.
- The accessor contract of `StyleConfig.h:793-885` is untouched, so no backend changes.
- The cost is observable in the test suite, and the before/after figures are recorded for the shipped
  stylesheets.

**Non-Goals:**

- The `TypeInfoSet` representation (TODO §20) and with it the `*TypeSets` members. They stay as they are;
  those files are live in three open PRs, and the representation is a separate change.
- The other type-count-indexed tables outside `StyleConfig`: `LabelProvider.cpp:31-46` (`nameLookupTable`,
  `nameAltLookupTable`) and `StyleConfig::wayPrio` (`:1110`) are left alone.
- Unparking the parked type set (TODO §74) and the style-switch cache invalidation question (TODO §53).
- Any style-resolution cache in a painter; this change is about what a load builds, not about per-frame
  resolution.

## Decisions

All file references are to `libosmscout-map/src/osmscoutmap/StyleConfig.cpp` unless stated otherwise.

### D1 — Index the per-family tables by the referenced types, not by the defined types

The postprocess step first collects the set of types the conditionals of a family name (they already walk
every conditional × type at `:520-541` and `:556-571`), then builds the table over that set only, with a
per-family translation from the global type index to the local position. A lookup for a defined but
unreferenced type takes the "no entry" path of D3.

- **Chosen**: keep `std::vector<std::vector<std::list<StyleSelector<S,A>>>>` as the table type, but size the
  middle dimension by the referenced types and add one `std::vector<uint32_t>` (or `size_t`) translation
  array of `typeConfig.GetTypeCount()` entries per family. The lookup keeps a single O(1) indirection and
  the per-level lists stay contiguous.
- **Alternative: `std::unordered_map<size_t, PerTypeTable>` keyed by the type index.** Fewer arrays, no
  translation vector, and the map holds only referenced types. Rejected: a hash lookup on the hot
  per-object path (every point label and every area fill resolves through it) is slower than an index into
  a small array, and it makes the table iteration order of `HasStyle`
  (`:672-684`) and `PostprocessVisibilityBounds` (`:1036-1066`) non-deterministic, which the visibility
  bounds do not currently depend on but the tests pin.
- **Alternative: keep the dense type × level grid and only share one empty list for level entries with no
  rule.** Least invasive, keeps every access path literal. Rejected: it removes the mostly-empty `std::list`
  containers but not the per-type `std::vector` of levels, so both the build walk and the retained memory
  still scale with the type count — the requirement "Defined but unreferenced types add no build work"
  would fail.
- **Alternative: build the tables lazily on the first lookup of a type.** Rejected: the configuration is
  shared and read concurrently by render jobs, so lazy construction needs a lock on the resolution path; and
  the measurement requirement could no longer report a build cost.

**Risk**: the translation array is indexed by a global type index that comes from a `TypeConfig` the
configuration was built from; a configuration built from a different type configuration with more types
would read past it. Mitigation: size the translation array from the same `typeConfig` the tables are built
from and keep the existing "built from this type configuration" invariant of `StyleConfig`; the unit test
pins a resolution on a type whose index is the highest defined one.

### D2 — Keep the accessor signatures of `StyleConfig.h:793-885`

- **Chosen**: internal-only change. `GetNodeTextStyles`, `GetNodeIconStyle`, `GetWayLineStyles`,
  `GetWayPathTextStyle`, `GetWayPathSymbolStyle`, `GetWayPathShieldStyle`, `GetAreaFillStyle`,
  `GetAreaBorderStyles`, `GetAreaTextStyles`, `GetAreaIconStyle`, `GetAreaBorderTextStyle`,
  `GetAreaBorderSymbolStyle`, `GetRouteLineStyles`, `GetRoutePathTextStyle` keep their declarations and
  their results.
- **Alternative: return a view/span over the resolved selectors instead of filling a caller vector.**
  Rejected for now: it is the better long-term shape, but it changes every caller, including
  `MapPainter.cpp`, which PR #1784 rewrites right now — a hard conflict for no requirement in this change.
- **Alternative: add parallel accessors and deprecate the old ones.** Rejected: two resolution paths to keep
  behaviourally identical, and the deprecation has no consumer pressure.

**Risk**: the table members are private, but `GetAreaFillStyleSelectors` at `StyleConfig.h:901` and the
`*TypeSets` members are public, so a shape change leaks through them. Mitigation: leave those signatures and
their semantics as they are, and cover them in the test's resolution cases.

### D3 — An unreferenced type resolves to "no style" through the translation table

The lookup translates the type index to "not referenced" and returns no style, which also removes
`assert(!styleSelectors.empty())` (`:1273`) and the matching assert in `HasStyle` (`:677`).

- **Chosen**: the translation maps an unreferenced type to a sentinel that the lookup treats as "no
  selectors", so no `StyleSelector` list is ever built for it and the walk in
  `PostprocessVisibilityBounds` skips it.
- **Alternative: an early `return nullptr` in each accessor before the table access.** Rejected: it spreads
  the same check over fourteen accessors and leaves `PostprocessVisibilityBounds` and `HasStyle` with the
  old assumption.
- **Alternative: keep the asserts and require callers to ask a `Has…` predicate first.** Rejected: every
  backend would have to add the check, `AGENTS.md` treats an assert that user stylesheet data can trigger as
  a bug, and the stylesheets in the tree already reference parked types that are not defined.

**Risk**: the sentinel path changes how a type the stylesheet does not reference is looked up. Today every
*defined* type owns a level vector whose entries are empty lists, so such a type also resolves to no style
and `assert(!styleSelectors.empty())` never fires for it (verified 2026-10-03: that assert is unreachable for
a defined-but-unreferenced type, it would only fire for a type the type configuration does not know); after
the change such a type has no position in the table at all. Mitigation: the translation array maps it to the
sentinel, and the resolution case pins "no style, no assert" in a Debug and in a Release build.

### D4 — Measure with a unit test that reports a slot count, plus a timing figure

- **Chosen**: a new `Tests/src/StyleConfigLookupCostTest.cpp` (the existing style tests are one file per
  concern: `StyleConfigSymbolsTest.cpp`, `StyleConfigVisibilityBoundsTest.cpp`, `StyleLoadResilienceTest.cpp`)
  covering the resolution contract, the unreferenced-type case, the scaling property against an extended type
  configuration, and a timing figure for the shipped stylesheets. The numbers it asserts are deterministic:
  the prepared slot count, the number of type-condition evaluations the build performed, the allocation count
  and the retained table bytes, read through a small build diagnostic on the configuration (in the spirit of
  `AreaIndex::GetEntryCount()` and `MapPainterCairo`'s resolved-font counter). Timing is reported, not asserted
  tightly, following `Tests/src/TypeResolutionPerformanceTest.cpp`.
- **Alternative: extend `Demos/src/SymbolsAll.cpp --list` and read the figure by hand.** Rejected as the only
  path: it is the natural place for a human-readable figure, but nothing fails when the cost regresses.
- **Chosen complement**: `SymbolsAll --list` prints the build cost for the shipped stylesheets as well, so
  the before/after figure in `verification.md` and the CI-visible test come from the same measurement code
  path.

**Risk**: in the `38-49 ms` figure of TODO §22 the type definition parse dominates, so an end-to-end timing
assertion is noisy. Mitigation: assert on the deterministic numbers (prepared slots, type-condition
 evaluations, allocation count, retained table bytes) and report the timing without a tight assertion,
 stating the tolerance and repeating the run (measured spread at the larger type set: 217-310 ms over five
 runs).

### D5 — The named-type list comes from the parse, not from enumerating the type configuration

- **Chosen**: the parser already resolves each rule's `TYPE` names to `TypeInfoRef`s
  (`libosmscout-map/include/osmscoutmap/oss/Parser.h:211`). Keep that resolved list on the rule's filter — a
  `std::vector<TypeInfoRef>` beside the existing `TypeInfoSet`, exposed by an accessor on `StyleFilter`
  (`StyleConfig.h:243-281`) — and drive `SortInConditionals` (`:544-571`) and `CalculateUsedTypes` (`:520-541`)
  from it, collecting each family's referenced set from the same lists. Every parsed filter carries a type
  set — `TYPE`, `GROUP`, `FEATURE` and `PATH` each call `SetTypes()` (`oss/Parser.cpp:1140`, `:1152`,
  `:1170`, `:1219`) — so no family needs an all-types fallback, and a filter that never received `SetTypes()`
  covers no type at all, because `TypeInfoSet::IsSet` is bounds-checked (`TypeInfoSet.h:160-166`). A
  family's referenced set is therefore exactly the union of its filters' type sets.
- **Alternative: enumerate each filter's `TypeInfoSet`.** This is what the loops do today (`HasType`).
  Rejected: `TypeInfoSet` is a full-length `std::vector<TypeInfoRef>`, so that enumeration is O(defined types)
  again and leaves the walking term of "Expected effect" untouched — the load time would keep growing with
  the type count while the prepared-slot assertion passes.
- **Alternative: pre-scan the stylesheet text for the type names of each rule.** Rejected: resolving the
  names a second time can disagree with the parse and re-implements the parser's name resolution.
- **Alternative: ask the `TypeConfig` for the types that carry a style attribute.** Rejected: type
  capabilities say nothing about which rules a stylesheet has; a type without a rule would gain a table.

## Expected effect (measured)

Measured 2026-10-03 with `Demos/SymbolsAll --stylesheet stylesheets/standard.oss --ost <type file> --list`,
median of five runs, same stylesheet and same parked rules in both runs, the second type configuration being
the shipped `stylesheets/map.ost` with its `PARKED-NEW-TYPE` blocks uncommented into a `/tmp` copy whose
sibling `.ost` files are symlinked (so the referenced-type set is identical and only the defined-type count
moves). These are end-to-end process times, not the load portion quoted in TODO §22 — they show the slope,
not the absolute figure.

```
defined types   median     min      max      unresolved-type warnings
            638        103.4 ms   93.7    118.5    3
           1489        232.9 ms  217.3    309.7    3      -> +129 ms / x2.25
```

Where the slope sits (both terms scale with the defined type count today):

```
term                                        site                              shipped                1489 types
A allocation  types x (levels+1) lists     StyleConfig.cpp:550-554           ~350 KB per family-slot ~820 KB
                                            (x slots: :608-632)
B walk        conditionals x all types      :556-571 (HasType)                603 x 638  = 385k       603 x 1489 = 898k
B walk x levels  (CalculateUsedTypes)       :520-541                          x ~21 = ~8.1M           x ~21 = ~18.9M
B walk x levels  (visibility bounds)        :1036-1066                        types x levels          x2.33
```

Scale of the referenced set in the shipped stylesheets: 603 `TYPE` selectors, 471 distinct names, **423
resolvable** of the 638 defined; the 48 unresolvable ones are runtime types (`_favorite`, `_route`,
`_osm_tile_border`, `_search_selected`, …), and 38 selectors name no type at all.

Projected effect:

- **Cost**: the build stops paying `conditionals x defined types` and pays `conditionals x types named by that
  conditional` (554 resolved name applications instead of 385k membership tests, and the level dimension
  follows), so the slope of the measured table goes away. **Measured (2026-10-03, same session, median of
  five end-to-end `SymbolsAll --list` runs)**: 168.4 ms at 638 defined types and 234.7 ms at 1489 before the
  change against 28.8 ms and 56.7 ms after, i.e. the slope per +851 defined types fell from +66.3 to +27.9 ms
  and the shipped figure fell 5.8x. The prediction that the shipped figure would barely move was wrong: the
  type-count-driven walk and the per-type level containers were a much larger part of the load than the type
  definition parse. The remaining slope is the `*TypeSet` vectors (TODO §20) and the larger `.ost` parse.
- **Memory**: the shipped configuration's style-selector tables now retain 3 845 544 bytes for 152 560 prepared
  slots (measured by the test and by `SymbolsAll --list`), against the ~4.7 MB the dense shape held at the
  same type count; at 1489 defined types the dense shape is ~820 KB per family-slot against ~233 KB for the
  same referenced types. The `*TypeSet` vectors are reported separately and still scale (615 600 bytes at the
  shipped type count), because their representation is TODO §20.
- **Lookup**: one extra dependent load (the translation array is 638 x 4 B ≈ 2.6 KB at the shipped type count,
  ≈ 6 KB at 1489, so it stays in L1/L2) per style family per resolved style. **Measured**: 11.4 ns per
  resolution of a referenced type and 1.9 ns for a type the sheet does not reference (200 000 iterations each),
  the latter cheaper because it takes the not-referenced branch instead of walking a level's selectors.
- **Not removed**: the `*TypeSets` members stay type-count sized until TODO §20.

## Flow

```text
loadStylesheet(path)
  |
  v
StyleConfig::Load/StyleConfig  -- parses .oss into conditionals per style family
  |                              (nodeText, nodeIcon, wayLine, wayPathText, wayPathSymbol,
  |                               wayShield, areaFill, areaBorder, areaText, areaIcon,
  |                               areaBorderText, areaBorderSymbol, routeLine, routePathText)
  v
Postprocess()
  |-- PostprocessNodes / Ways / Areas / Routes          (:634, :686, :755, :865)
  |     |-- GetMaxLevelInConditionals  -> maxLevel
  |     |-- collectReferencedTypes(conditionals)        <-- new: the types the family names
  |     |-- SortInConditionals / ...BySlot               (:544, :608)
  |     |     |-- table over referenced types only       <-- CHANGED (was GetTypeCount())
  |     |     '-- translation[global type index] -> local position | not-referenced
  |     '-- CalculateUsedTypes -> *TypeSets              (unchanged, §20 out of scope)
  |-- PostprocessVisibilityBounds                         (:1036) walks the referenced types only
  '-- PostprocessIconId / PostprocessPatternId / PostprocessVisibilityBounds
  v
lookup: MapPainter -> StyleConfig::GetNodeTextStyles / GetAreaFillStyle / ...  (:1337, :1385, :1551, :1589)
  |-- translation[type->GetIndex()] --> local position or not-referenced
  |     '-- not-referenced --> no style, no entry built, no assert
  '-- GetFeatureStyle(selectors[position])               (:1267)
        |-- level = projection magnification, clamped to the family's level count
        '-- iterate the selectors of that level, apply criteria, compose attributes
```

## Risks / Trade-offs

- **Lookup latency regression from the indirection** → the translation is one contiguous array read on the
  same cache line the lookup already touches; the timing case in `StyleConfigLookupCostTest.cpp` measures
  resolution as well as the build, and the full `PerformanceTest` suite is run before the change is
  accepted.
- **Level-clamp behaviour changes because the level dimension shrinks** → keep each referenced type's level
  vector at `maxLevel+1` for its family and pin the highest-level resolution with a test case (the clamp at
  `:1282-1284` currently resolves the last level for a magnification beyond the table).
- **`HasStyle` (`:672-684`) and `PostprocessVisibilityBounds` (`:1036-1066`) keep the old assumption** → both
  are adapted in the same step as the table shape and covered by `StyleConfigVisibilityBoundsTest.cpp`,
  which must stay green without edits.
- **Only the table shape changes while the walk keeps scanning every defined type** → the prepared-slot
  assertion would pass while the load time still grew with the type count (measured: 103 ms at 638 defined
  types against 233 ms at 1489 with the same stylesheet). Mitigation: the build diagnostic counts
  type-condition evaluations as well, and the scaling case compares one stylesheet against two type
  configurations, so the walking term is asserted rather than inferred.
- **The type-count part of a loaded configuration is not fully removed by this change** → `nodeTypeSets` and
  friends (`:653-666`, `:719`, `:826`, `:879`) stay `TypeInfoSet`-per-level (≈450 KB at 21 levels and 638
  types) until TODO §20 changes the set representation, which is live in three open PRs. Mitigation: the
  measurement reports table bytes separately from the `*TypeSets`, and TODO §74's cost gate is not treated as
  closed by this change alone.
- **A stylesheet that references an undefined type** → the referenced set is collected from resolvable types
  only (unresolved names are already reported during parsing); the new scenario pins that the remaining rules
  still resolve, complementing `StyleLoadResilienceTest.cpp` and the shipped-stylesheet symbol test.
- **The measurement baseline is confusing because the parked type set is inactive** → `verification.md`
  records the figure for the shipped type configuration and, separately, the slot count with and without a
  synthetic type set extension, so the type-count independence is shown without unparking anything.
- **Test registration in both build systems** → the new test file is added to `Tests/CMakeLists.txt` and
  `Tests/meson.build` in the same commit as the file, as the previous changes did.

## Migration Plan

- No data, format or public-API migration: the change is confined to how a loaded style configuration is
  built and read at runtime, so databases and stylesheets in the field are unaffected and no re-import is
  needed.
- Deployment is the library itself: a consumer that rebuilds against the new library gets the smaller
  configuration; a consumer that pins an older library keeps the dense tables.
- Rollback is reverting the commit; the tables return to their current shape and no cleanup step is needed.
- Ordering: land this change before the parked type set is enabled (TODO §74), because that unpark is what
  makes the cost visible to users.

## Open Questions

- Whether the same referenced-type indexing should later be applied to `LabelProvider.cpp:31-46` and
  `StyleConfig::wayPrio` (`:1110`), which are type-count sized in the same way. Deferrable: it changes no
  requirement of this change and is recorded in `TODO.md` if it stays open.
- Whether the visibility-bounds pass should be folded into the table build instead of walking the tables a
  second time. Deferrable: it is inside the same file and the same step, and the requirement only asks that
  the cost not follow the defined type count.
