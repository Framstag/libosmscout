# Design — cleanup-oss-base-files

## Context

See `proposal.md` — Why. Key constraints discovered during investigation:

- OSS rule resolution: `StyleConfig::GetFeatureStyle` iterates all matching selectors in **file order** and `CopyAttributes` — a later rule **overrides** an earlier rule for overlapping attributes (fill color, etc.). Rule order is therefore load-bearing: moving rules between positions can change which rule wins.
- `[GROUP a, b]` is a set intersection (`Parser.cpp` `STYLEFILTER_GROUP`: collect group `a`, drop types not in group `b`).
- CONST and SYMBOL definitions must be unique per style sheet: the parser raises `Constant already defined` (`Parser.cpp:697`) and `Map symbol ... already defined` (`Parser.cpp:242`). A style sheet cannot include a module whose CONST/SYMBOL names collide with its own — duplicates must be removed from the parent.
- Existing verification: `Tests/CMakeLists.txt` registers `CheckStyleSheet-<name>` tests for all 7 touched base files (standard, winter-sports, boundaries, railways, motorways, public-transport, cycle) via `OSTAndOSSTest --warning-as-error` with `map.ost`. `PerformanceTest` exists for render comparison.

## Goals / Non-Goals

**Goals:**
- Every base file selects building types via `[GROUP building]` or the module system instead of hand-maintained explicit type lists; adding a new `*_building` type to `map.ost` (with the established `GROUP building` convention) requires zero `.oss` edits.
- Remove duplicated rule blocks and CONST/SYMBOL definitions between base files and `include/` modules (cycle.oss ↔ amenity/sport/leisure/natural/waterway/landuse/power; standard/cycle/winter-sports ↔ each other).
- Identical rendering for existing types; only reviewed, intentional deltas allowed.

**Non-Goals:**
- No change to `railways.oss`, `motorways.oss`, `coastlines.oss`, `basemap-render.oss` (already minimal/intentionally custom).
- No change to public-transport's overall look (place/waterway/highway/railway rules stay custom).
- No new rendering features, no API/data-format changes.

## Decisions

### D1. Complete the `building` GROUP in `map.ost`

Add `building` to the GROUP of the 14 building types that lack it: `building`, `building_garage`, `landuse_farmyard_building` (currently `GROUP landuse`), the 8 religious building types (`temple_building`, `shrine_building`, `christian_cathedral_building`, `christian_chapel_building`, `christian_church_building`, `jewish_synagogue_building`, `muslim_mosque_building`, `worship_building` — all `GROUP religious, routingPOI`), `leisure_building`, `sport_building`, `military_bunker_building`.

- Rationale: establishes the invariant "all renderable building types carry `GROUP building`", making `[GROUP building]` a complete selector. Verified zero output change today: the only `[GROUP …, building]` users are `historic/office/shop/tourism` modules, whose category groups do not overlap the 14 types; `[GROUP building]` alone is unused anywhere.
- Alternative A (rejected): keep `map.ost` untouched and maintain explicit tail lists (religious/leisure/sport/military/farmyard/garage/generic) in every base file — leaves the exact maintenance trap this change removes.
- Alternative B (rejected): introduce a synthetic group name — unnecessary; `building` already exists and is the convention.

### D2. public-transport.oss: building rule → `[GROUP building]`

Replace the ~40-name explicit list (lines 330-371) with a `[GROUP building]` selector, keeping the block's `[MAG veryClose-]` fill/border and the nested `[GROUP amenity]` pink override and the separate `[TYPE shop]` block unchanged.

- Rationale: after D1, `[GROUP building]` covers every type in today's list plus all future `*_building` types (the trigger case: `tourism_*_building` additions needed no edit here anymore).
- Rule order: `[GROUP building]` block is the only building-fill rule in this file (plus its nested amenity override, which is later in the same block → still wins for amenity buildings; the `[TYPE shop]` block after it is a disjoint type). Last-wins semantics preserve today's look.
- Deltas (reviewed, intended): `shop_*_building` areas gain the gray `#f0f0f0` fill (currently unstyled); any future building type is styled automatically.
- Alternative (rejected): include the category modules and reuse their fills — public-transport's building palette (gray `#f0f0f0`, pink `#ffe0e0`) deliberately differs from module colors (`@buildingColor` `#d9d9d9`, `@amenityBuildingColor` `#d9b8b8`).

### D3. cycle.oss: adopt `natural`, `waterway`, `amenity`, `sport`, `power`, `buildings` modules

cycle.oss inlines near-verbatim copies of several modules (verified: 61/62 normalized lines of the inline waterway section match the module; natural/amenity/sport/power are subsets with identical CONST values) but includes none of them. Change:

- Add the 6 `MODULE` directives (natural, waterway, amenity, sport, power, buildings) after `include/tourism`.
- Delete the inline sections covered by those modules: natural, waterway, and the buildings/amenity/sport/power block (keeping cycle's leisure building bits: leisure_stadium, leisure_sports_centre/leisure_building fill, leisure_building + leisure_pitch/fitness_station text).
- Delete colliding CONST definitions from cycle's CONST block: 19 natural names and 10 amenity names (values verified identical to module definitions). Landuse/leisure CONSTs stay (sections kept inline).
- Delete cycle's duplicate SYMBOLs `natural_peak`, `natural_volcano`, `stream_arrow` (module versions identical).
- Add `UINT labelPrioSpring = 36;` (required by the natural module, was missing).
- Keep cycle-specific content: synthetic/contours, highway overrides, landuse, leisure, aerialway (aerialway differs from the module by design), routes.

**Deviation from original plan (adopt 7 modules incl. landuse+leisure):** diff showed cycle's landuse (farmColor, village_green/orchard `#fafdf7` vs module `#cfeca8`, different MAG thresholds) and leisure (park `#dbf5e0` vs `#c6f0cf`, golf `#e9fadc`, garden `#eff9e2`, playground `#ccffff` vs `#affdbb`) are intentionally different palettes, not stale copies. Porting them into shared modules would change standard.oss/winter-sports.oss, so those sections stay inline; the adopted modules are strict supersets of cycle's copies (reviewed deltas: dock/stream fill MAG cityOver→suburb, pattern/emphasizeColor additions, amenity label halo, 7 extra amenity types).

### D4. New module `include/buildings.oss`

Extract the generic building block triplicated in standard.oss (~lines 215-270), cycle.oss (883-973 core), winter-sports.oss (~330-380): farmyard fill/border, generic `building` fill/border, `building_garage` fill, address labels, building-name label, farmyard text label. Parameterized by parent-file CONSTs (`@buildingColor`, `@buildingBorderColor`, `@buildingLabelColor`, `@buildingMag`, `@minorBuildingMag`, `@labelBuildingMag`) and flags (`_building`, `_minorBuilding`) — all already defined in all three parents.

- Keep **exact-type** selectors (`[TYPE building]`, `[TYPE landuse_farmyard_building]`, `[TYPE building_garage]`), not `[GROUP building]`: inside standard.oss/cycle.oss/winter-sports.oss the module's fill rule must not match `amenity_*_building` etc. — those are styled by their category modules, and last-wins means a later `[GROUP building]` fill would override them.
- Unify the one small divergence: farmyard text label gets `color: @buildingLabelColor` (standard.oss variant; winter-sports/cycle currently omit it) — reviewed delta, darkens farmyard labels from `@labelColor` to `#7a7a7a`-ish.
- Placement: add the `MODULE "include/buildings"` line after `include/power` (last), before `STYLE`; relative order of the extracted rules is preserved from standard.oss, and the type sets are disjoint from all module rules.
- Rationale: removes ~30 lines × 3 files of near-identical rules; keeps the generic fill catch-all behavior identical.
- Alternative (rejected): `[GROUP building]`-based generic fill inside the module — would override category module fills (last-wins) and flatten the palette.
- Alternative (rejected): leave triplicated — no benefit.

### D5. winter-sports.oss: drop rules now covered by modules

Delete from winter-sports STYLE: `[TYPE leisure_stadium]` block (duplicate of `include/leisure.oss`), `[TYPE leisure_sports_centre, leisure_building]` fill block, `[TYPE leisure_building]` text rule. Replace the remaining building block with `MODULE "include/buildings"` per D4.

- Verified `include/leisure.oss` provides identical rules (stadium `#33cb98`, sports_centre/leisure_building `@buildingColor` fill, leisure_building text).
- Rationale: winter-sports already includes the leisure module; these STYLE rules are dead duplicates.

### D6. boundaries.oss: reuse `include/land_sea`

Replace inline tile rules (lines 12-15) with `MODULE "include/land_sea"`; keep the `@waterColor`/`@landColor`/`@unknownColor` CONSTs (module consumes them). railways.oss/motorways.oss already follow this pattern.

- Rationale: removes duplicated tile styling; module is textually identical.

### D7. Deferred (documented, not in this change)

- `include/religious.oss` / `include/landuse.oss` still use explicit building lists; once D1 lands they can collapse to `[GROUP religious, building]`-style selectors.
- Shared `include/place_basic.oss` for the minimal debug styles (railways/motorways/boundaries) — their place rules intentionally differ from `include/place.oss`.

## Risks / Trade-offs

- **Rule-order regression** → All moved rules are either disjoint in type set from surrounding rules or keep relative order; `GetFeatureStyle` last-wins verified; every touched file is covered by `CheckStyleSheet-*` tests which fail on parse errors.
- **Incomplete CONST/SYMBOL removal in cycle.oss** → `Constant already defined` / `Map symbol already defined` parse errors; `CheckStyleSheet-cycle.oss` catches immediately. Task D3 lists all colliding names explicitly.
- **Visual delta beyond intent** (cycle gains module-extra rules, amenity label halos; winter-sports farmyard label color; public-transport shop building fill) → Each delta enumerated in tasks; verify via `PerformanceTest` render comparison on a fixed extract before/after.
- **D1 changes type metadata others may rely on** → Only GROUP additions to existing types; no type set membership used by code (`TypeInfoSet`) changes; intersections verified disjoint. `map.ost` change is additive.
- **public-transport.oss stays custom** → Future type additions outside `building` group (e.g. a new `_platform` type) still need manual edits there; accepted — scope is building-type lists.

## Migration Plan

1. D1 (`map.ost`) first — additive, unblocks GROUP selectors.
2. D4 (`include/buildings.oss`) + D5 (winter-sports) + D3 (cycle) + D6 (boundaries) — per-file edits; run `CheckStyleSheet-*` per file.
3. D2 (public-transport) last — largest behavioral surface (new gray fills).
4. Render comparison with `PerformanceTest` (standard, cycle, winter-sports, public-transport) on a fixed extract; review deltas from the enumerated list.
5. No rollback complexity: all changes are stylesheet/metadata edits revertible via git; no data or code migration.

## Open Questions

None blocking. Render-delta acceptance (exact pixel equality vs. reviewed improvement for the enumerated deltas) is a judgment call during verification, not a design change.
