# Design

## Root cause

Two import steps enumerate the types they treat as administrative boundaries by name, and each list was
written when `map.ost` carried four such types. `cd273c581` split two more out of the generic rule and
declared them **before** it, so type resolution (lowest `typeIndex` first,
`libosmscout/src/osmscout/TypeConfig.cpp:1194`, `:1227`) now hands a level-8 area
`boundary_municipality` and a level-10 area `boundary_suburb`:

| step | set | effect of a missing name |
|------|-----|--------------------------|
| `GenLocationIndex::Import` (`:2761-2783`), read by `GetBoundaryAreas` (`:1245`) | country, state, county, administrative | the area is skipped above every `ReportLocation` call — silent in all import reports |
| `GenRelAreaDat::ResolveMultipolygonMembers` (`:651-673`), read at `:697` | the same four | a nested boundary relation member takes the generic path and is dropped with `Warning: Unsupported relation (...) reference` (`:717`) |

Before that commit neither type existed and both levels fell through to `boundary_administrative`, which
both lists carry. `ImportParameter`'s `maxAdminLevel` default of 10 (`ImportParameter.cpp:104`) lets both
levels through the scan, so the limit is not part of the cause.

## The single resolver

A new private helper of the import library owns the list:

```cpp
TypeInfoSet ResolveBoundaryTypes(const TypeConfig& typeConfig,
                                 Progress& progress);
```

- It builds a `TypeInfoSet` from the type definition's type count and adds each declared boundary type.
- The four legacy names stay required: a type definition without them cannot produce a region tree at
  all, and the pipeline asserted them before. Missing one still trips the assertion.
- `boundary_municipality` and `boundary_suburb` are resolved optionally: a type definition that predates
  them resolves level 8 and 10 to `boundary_administrative`, which is in the set, so those boundaries are
  still regions. The helper reports the absence through `Progress::Debug`, not `Warning`: an older type
  definition is a supported input, not a defect of the data, and a warning per import of every older map
  would be noise.
- Both steps call it, so the two lists cannot disagree: a type added to the resolver reaches the index and
  the relation composition at once.

```
TypeConfig (map.ost)
      |
      v
ResolveBoundaryTypes  -- required: country/state/county/administrative (assert)
      |               -- optional: municipality/suburb (Debug when absent)
      |
      +--> GenLocationIndex::Import        -> GetBoundaryAreas -> regions of location.idx
      |
      +--> GenRelAreaDat::ResolveMultipolygonMembers -> ComposeBoundaryMembers vs ComposeAreaMembers
```

## Alternatives

**A. Add the two names to both lists (the local patch).** Smallest diff, and it fixes this defect. Rejected
as the shape of the fix: the duplication is what produced the defect, a third step that needs the list
would start from the same four names again, and nothing in the tree would report the disagreement. It also
keeps the assert on the two new names, which aborts a debug import of every type definition older than
`cd273c581` — the requirement that such a definition still imports cannot hold with it.
*Risk:* reoccurrence on the next stylesheet split; measured already once.

**B. Derive the set from the `ADMIN_REGION` flag** (`TypeInfo::GetIndexAsRegion()`, the flag
`GetRegionAreas` uses for `place=*` areas, `GenLocationIndex.cpp:1371`). Rejected: `place_city`,
`place_town` and the other place types carry `ADMIN_REGION` and are not boundaries. Feeding them to
`ResolveMultipolygonMembers` would compose a city area out of its relation members as a boundary, and
`GetBoundaryAreas` would then read `admin_level` from place areas that do not carry it, reporting
`No tag 'admin_level'` for every one of them. The boundary set is narrower than the region flag by design.
*Risk:* corrupt area composition and a flooded import report on every map.

**C. One resolver, two call sites (chosen).** One list, one policy, testable without an import run, and the
`GenRelAreaDat` step is covered by the same unit case as the index step. *Risk:* the helper becomes a
public-ish interface of the import library; mitigated by keeping it in `include/osmscoutimport/private/`
next to `AreaWayIdReferenceRule.h`, so it is not installed and not part of the published API.

The optional resolution is a separate decision: **assert** (what the local patch does) aborts a debug
import of a pre-`cd273c581` type definition and dereferences null in a release build, because
`TypeInfoSet::Set` asserts and then reads `type->GetIndex()`
(`libosmscout/src/osmscout/TypeInfoSet.cpp:65`, measured: `evidence/probe-assert-legacy-typeconfig.log`);
**skip silently** is the failure mode this change is about. Chosen: skip with `Progress::Debug`, which
keeps the run alive, keeps the fallback behaviour of an older definition and leaves a line for anyone
asking where the region went.

## Test strategy

- `BoundaryTypesTest` (unit, no import): the shipped `stylesheets/map.ost` resolves all six types; a
  minimal type definition with only the four legacy types resolves the four, resolves neither new name and
  trips no assertion. The second case is the one that fails against the local patch.
- `LocationBoundaryRegionTest` (end-to-end, in the existing `LocationLookupTest` binary, which imports
  `Tests/LocationTest.olt` with the shipped type definition): a level-8 boundary area and a level-10 one
  nested in it are present as regions and carry the parent chain the fixture describes. This is the case
  that fails while the index step's set lacks the two names; it is red against the pre-change tree.
- Not covered end-to-end: the relation composition of a nested boundary relation member. `*.olt` generates
  ways, and the repository has no relation fixture for a boundary. The resolver unit case covers the list
  both steps read; the composition path itself stays untested here and is recorded in the change's
  `verification.md`.

## Risk assessment

- **Region hierarchy of existing databases**: unchanged for every level the old lists already covered; the
  two new levels only add regions that the stylesheet always meant to index. A map imported before this
  change stays as it is until it is rebuilt.
- **Type definitions without the new types**: no behaviour change beyond a debug line.
- **Relation composition**: a level-8 relation's members are now composed as boundary members. For a
  relation whose members are ways this is the same composition as before (both paths collect `outer` and
  `inner` ways); for a nested relation member it now recurses instead of dropping the member — the intended
  behaviour and the reason the set is shared.
- **Reports**: `location.html` still reports `No name`, `No tag 'admin_level'` and the level limit as
  before. This change deliberately does not add a report for "type not in the boundary set", because with
  the resolver that cannot happen for a declared type; an undeclared one is reported as a debug line by the
  resolver.
