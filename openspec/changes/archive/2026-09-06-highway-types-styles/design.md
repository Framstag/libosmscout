# Design: highway-types-styles

## Context

See proposal.md — Why. The type definition file (`map.ost` + module `motorways.ost`) lacked many documented `highway=*` values, so those objects were not imported, rendered, or routable. Implementation is complete; this document records the design decisions made.

## Goals / Non-Goals

**Goals**
- Cover every documented `highway=*` value with ≥1000 uses in OSM that is not discouraged
- Match NODE/WAY/AREA object types to the OSM wiki element tables
- Add style definitions where visualization is obvious
- Keep changes confined to stylesheet files (`.ost`/`.oss`) — no C++ changes

**Non-Goals**
- Adding values with <1000 uses or discouraged status
- Adding icons as new image files (geometric SYMBOLs used instead)
- Changing routing engine behavior (PATH flags only, no new vehicle types)

## Decisions

### D1: Which values to add
**Decision:** Add values from taginfo with `count >= 1000` and `in_wiki == true`, excluding discouraged values (no, yes, planned, disused, abandoned, razed, piste, ford, turntable, residential_link, traffic_calming, sidewalk, traffic_island).

**Alternatives:**
- Add all wiki-documented values regardless of usage — rejected: would add near-unused types (e.g. `traffic_island` 855, `sidewalk` 633) with no data to render.
- Add only values above a higher threshold (e.g. 100k) — rejected: would miss relevant features like `ladder` (2.5k) and `escape` (2k) that are legitimately mapped.

**Rationale:** 1000 uses is the user-specified relevance threshold; wiki status confirms the value is a real, non-deprecated tag.

### D2: Type placement
**Decision:** New WAY types (busway, raceway, escape, proposed, corridor) go in `stylesheets/motorways.ost` (the module for road types, included by `map.ost`); new NODE types go directly in `stylesheets/map.ost` in the highway section.

**Alternatives:**
- All types in `map.ost` — rejected: breaks the existing module organization where road types live in `motorways.ost`.
- New module file for the node types — rejected: no precedent; node types are already inline in `map.ost`.

**Rationale:** Follows existing file organization; keeps related types together.

### D3: Style approach for node types
**Decision:** Define geometric SYMBOLs inline in `stylesheets/include/roads.oss` and reference them via `NODE.ICON { symbol: ... }`.

**Alternatives:**
- Add PNG/SVG icon files to `libosmscout/data/icons/` — rejected: requires new image assets and build wiring; geometric symbols are self-contained and match the existing pattern (e.g. `highway_turning_cycle`, `mini_roundabout`).
- No styles for minor node types — rejected: user asked for styles where visualization is obvious.

**Rationale:** Self-contained, no new assets, consistent with existing symbol usage.

### D4: Street lamp day/night rendering
**Decision:** Two symbols — `highway_street_lamp` (GROUND glow, light on) and `highway_street_lamp_off` (lamp post pictogram, light off) — toggled by the existing `IF daylight` flag in the stylesheet.

**Alternatives:**
- Single symbol always — rejected: lamp glow looks wrong in daylight.
- New `_daylight` derived flag — rejected: `daylight` flag already exists and is the toggle; `_`-prefixed flags are derived category gates, not needed here.

**Rationale:** Uses the existing daylight mechanism; the `// TODO` in the stylesheet was explicitly reserved for this.

## Risks / Trade-offs

- [Symbol orientation] → Symbol coordinate system is y-down (screen coords); pole/sign arrangements must be authored accordingly. Verified via SymbolAll SVG output; 4 symbols corrected after review.
- [Type conflicts] → Some objects match multiple types (e.g. node with `highway=platform` + `public_transport=platform`). The type system assigns all matching types; no conflict handling needed.
- [Other stylesheets] → cycle.oss, public-transport.oss, motorways.oss do not style the new types; they simply render unstyled there. Accepted — standard.oss is the primary stylesheet.
- [Pre-existing uncommitted man-made work] → Working tree contained unrelated in-progress man_made type/style changes; left untouched to avoid mixing concerns.

## Migration Plan

No migration needed — stylesheet-only change. Rollback: revert the four stylesheet files. Validation: `OSTAndOSSTest` (map.ost + each stylesheet loads without new warnings), `ctest` suite (70/70 pass), `SymbolsAll` renders all symbols.

## Open Questions

None.
