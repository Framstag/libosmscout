## Context

See proposal.md - Why. The libosmscout stylesheets define worship types only for christian, jewish, and muslim; all other OSM religion values fall through to the generic `amenity` type. The existing `muslim_mosque_building` type has a `muslin` typo. The implementation is already complete in `stylesheets/map.ost` and `stylesheets/include/religious.oss`.

## Goals / Non-Goals

- Goals: add NODE worship types for all wiki-documented religion values (minus discouraged `none`), fix the `muslin` typo, add religion-specific symbols and styles, rename symbols with `religion_` prefix.
- Non-Goals: no AREA worship types (areas already covered by `temple_building`/`shrine_building`/`worship_building`), no changes to basemap.ost, no C++/build changes.

## Decisions

- **NODE-only worship types** — christian_worship is NODE-only; making the new types NODE AREA would shadow the generic `amenity` AREA styles for building-less place_of_worship areas (they would match the worship type but have no AREA style, so they would not render). NODE-only keeps the existing fallback behavior.
- **Symbols named with `religion_` prefix** — consistent naming for all religion symbols, including the pre-existing `christian_church_cross` (renamed to `religion_christian_church_cross`).
- **Muslim crescent follows OSM Carto shape** — the OSM Carto `Muslim-16.svg` path was sampled into a polygon (outer arc = left half of circle, inner boundary bulging right) because a concentric ring reads as a donut, not a crescent. The star is placed in the opening at top-right.
- **Generic `religion_place_of_worship` symbol for remaining religions** — hindu (Om), sikh (khanda), jain (hand), zoroastrian (faravahar) symbols are too complex for the RECTANGLE/CIRCLE/POLYGON primitive set; OSM Carto also uses a generic place-of-worship icon for these.
- **Symbols validated by rendering** — every symbol was rendered via `Demos/SymbolsAll` and pixel-checked; this caught two degenerate polygons (135°/315° dharma wheel spokes with all points collinear → zero area → nothing filled).

## Risks / Trade-offs

- [Polygon symbols are approximations of the OSM Carto icons] → verified against reference rasterizations; simple geometric shapes chosen where possible.
- [Symbols render differently across backends (Cairo/SVG/Qt/iOS)] → all use nonzero or odd-even winding; the crescent ring hole and star verified in Cairo (primary renderer).
- [Tiny religions (e.g. antoinist, 44 uses) get types] → cheap NODE definitions, consistent with the wiki-documented value list; no visual noise since they share the generic symbol.

## Migration Plan

No deployment steps — stylesheet-only change. Rollback: revert `stylesheets/map.ost` and `stylesheets/include/religious.oss`.

## Open Questions

None.
