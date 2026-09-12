# Design: place-types-and-styles

## Context

`stylesheets/map.ost` defines import-time feature types; `stylesheets/include/place.oss` defines their rendering. The place section covered 18 of the 33 documented `place=*` values; 17 documented values with real-world usage were missing and have been added (commit `5e20fa8e6`), and the 3 remaining documented values (`cadastral_community` 16,636 objects, `subcounty` 48 objects, `building_complex` 40 objects) have been added in this change. See proposal.md - Why for motivation and the spec for the full requirement list.

The `TypeConfig` is generated from the stylesheet at import time — no C++ code changes are needed. This is a pure stylesheet change, same pattern as the earlier `man-made-type-definitions`, `landuse-type-definitions`, `barrier-type-definitions`, and `waterway-type-definitions` changes.

## Goals / Non-Goals

**Goals:**
- Add 17 new `TYPE` definitions to the place section of `stylesheets/map.ost` (done) and 3 further definitions for the remaining documented values.
- Add label rendering rules in `stylesheets/include/place.oss` for all new types.
- Follow existing type/style conventions so the new definitions are indistinguishable in style from the current ones.

**Non-Goals:**
- No changes to existing type or style definitions.
- No C++/import-pipeline changes.
- No changes to other stylesheets (`standard.oss`, `cycle.oss`, `winter-sports.oss`).

## Decisions

### D1: Add types to the existing place section of map.ost

New `TYPE` definitions go into the `// Place` section of `stylesheets/map.ost`, between `place_ocean` and the `// Buildings` section, alphabetically ordered within the section.

- **Alternative considered**: a separate `place.ost` include file. Rejected — `map.ost` is the single source of type definitions; the prior type-definition changes all extended `map.ost` in place.
- **Rationale**: keeps type definitions discoverable in one place and matches established project convention.

### D2: Element types follow the OSM wiki element table

Each new type uses the element set from the wiki [Key:place](https://wiki.openstreetmap.org/wiki/Key:place) element table and the individual tag pages: `province`, `district`, `subdistrict` are node-only; `municipality`, `borough`, `quarter`, `neighbourhood`, `plot`, `isolated_dwelling` are node + area + relation; `archipelago` is area + relation; `ocean` is node-only; the rest (`farm`, `allotments`, `city_block`, `square`, `polder`, `sea`) are node + area. For the remaining values: `cadastral_community` is node + area (Czech cadastral communities are mapped as areas; the wiki element usage is unspecified, so the `place_county` pattern applies); `subcounty` is node + area + relation; `building_complex` is node + area with multipolygon relations allowed.

- **Alternative considered**: `NODE AREA` for everything. Rejected — would import relation-tagged `archipelago`/`municipality` objects incorrectly and node-tagged `ocean` objects, polluting the type config.
- **Rationale**: the spec requires wiki-conformant element types; import-time filtering is the cheapest place to enforce them.

### D3: Type flags follow the existing place conventions

Administrative and settlement types (`province`, `district`, `subdistrict`, `municipality`, `isolated_dwelling`, `farm`, `allotments`, `borough`, `quarter`, `neighbourhood`, `city_block`, `plot`) get `{Name, NameAlt, IsIn}` and `ADMIN_REGION`; relation-capable types additionally get `MULTIPOLYGON`. `locality` and `square` are `POI` (named, searchable places). `sea` gets `IGNORESEALAND`. `archipelago` gets `MULTIPOLYGON` without `ADMIN_REGION`. `island`, `islet`, `polder`, `ocean` carry only `{Name, NameAlt}`.

- **Alternative considered**: `ADMIN_REGION` on every type. Rejected — `island`, `islet`, `polder`, `sea`, `ocean`, `archipelago`, `locality`, `square` are not administrative regions; the flag would misclassify them in the region hierarchy.
- **Rationale**: matches the existing place conventions exactly; the flags drive import-time region indexing and label priority.

### D4: Label styles grouped by hierarchy in place.oss

New style rules in `stylesheets/include/place.oss` follow the existing label-priority scheme:

- New `labelPrio*` constants mirroring the existing ones: `labelPrioArchipelago` (3), `labelPrioPolder` (5), `labelPrioSquare` (14), `labelPrioProvince` (4), `labelPrioDistrict`/`labelPrioSubdistrict`/`labelPrioMunicipality` (5), `labelPrioBorough`/`labelPrioQuarter` (11), `labelPrioNeighbourhood`/`labelPrioCityBlock` (12), `labelPrioPlot` (13), `labelPrioIsolatedDwelling`/`labelPrioFarm`/`labelPrioAllotments` (13).
- `NODE.TEXT`/`AREA.TEXT` rules for each new type at the magnification level matching its hierarchy position: administrative types in the `[MAG region-city]`/`[MAG county-city]` blocks, settlement types in the `[MAG suburb-veryClose]`/`[MAG suburb-suburb]` blocks, small named places in the `[MAG closer-]` block, water/landmass types in the `[MAG region-veryClose]` block.
- The `[MAG region-city]` `AREA.TEXT` list is extended with the new settlement types.

- **Alternative considered**: one generic label rule for all new types at a single magnification. Rejected — provinces must not render at the same zoom as city blocks; the hierarchy-based priorities are what make the place label stack legible.
- **Rationale**: reuses the existing priority/magnification machinery; no new rendering concepts.

### D5: Remaining three types follow the same conventions

`place_cadastral_community` (`NODE AREA`, `{Name, NameAlt, IsIn}`, `ADMIN_REGION`), `place_subcounty` (`NODE AREA RELATION`, `MULTIPOLYGON ADMIN_REGION`), `place_building_complex` (`NODE AREA RELATION`, `MULTIPOLYGON`). Label rules: `cadastral_community` and `subcounty` in the `[MAG county-city]` block (administrative hierarchy), `building_complex` in the `[MAG closer-]` block (small named place, like `city_block`/`plot`).

- **Alternative considered**: skipping `subcounty` (48 objects) and `building_complex` (40 objects, experimental status) as too rare. Rejected — both are documented on the wiki with "in use" status and the change's selection criteria is documented values with any real usage; the cost of a type definition is one block of OST.
- **Rationale**: consistent with the man-made/landuse changes, which added documented values regardless of low usage when not discouraged.

## Risks / Trade-offs

- [New types may collide with existing type names] → Verified against current `map.ost`; all 20 names are new. Import fails loudly on duplicate type names, so a collision would be caught in CI.
- [Over-rendering at low zoom clutters the map] → All new label rules are gated behind the existing magnification blocks; nothing renders at low zoom beyond the administrative hierarchy that already renders there.
- [`building_complex` is experimental and may be retagged] → The type is additive; if the tag falls out of use the type definition can be removed without affecting other types.
- [Import-time behavior change requires database re-import] → Stylesheet-only change; existing databases keep working, new types appear after re-import. Rollback = revert the two stylesheet files.
