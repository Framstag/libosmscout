## Context

See proposal.md — Why. Current state: `stylesheets/map.ost` defines three aerialway types (`aerialway_gondola`, `aerialway_chair_lift`, `aerialway_drag_lift`) that fold multiple OSM values together. `stylesheets/include/aerialway.oss` provides way rendering (cable + carrier dash overlay) and labels for exactly those three types. Four documented values (`pylon`, `station`, `zip_line`, `goods`) have no type definitions, so they are dropped at import time.

The style system: `.ost` files define import-time types (element kinds, features, POI indexing); `.oss` files define rendering. `include/aerialway.oss` is included by `standard.oss`, `winter-sports.oss`, and `cycle.oss`; `public-transport.oss` has its own inline aerialway section. Each stylesheet has an `ORDER WAYS` group listing aerialway way types.

## Goals / Non-Goals

**Goals:**
- Add four new types to `map.ost` with correct element kinds per OSM wiki
- Add rendering for the new types in `include/aerialway.oss` following existing aerialway/railway style patterns
- Register new way types in all four stylesheets' `ORDER WAYS` aerialway groups

**Non-Goals:**
- No restructuring of existing aerialway types (e.g. not splitting `j-bar`/`t-bar`/`platter`/`rope_tow`/`magic_carpet` out of `aerialway_drag_lift` — they are already covered and rendered)
- No C++/import-pipeline code changes — types are picked up automatically from the stylesheet
- No icon/image assets — node symbols use the SYMBOL primitive system

## Decisions

### D1: Element kinds per type
Follow OSM wiki element tables exactly:
- `aerialway_pylon` = NODE (wiki: node only)
- `aerialway_station` = NODE AREA (wiki: node + area)
- `aerialway_zip_line` = WAY (wiki: way only)
- `aerialway_goods` = WAY (wiki: way only)

Rationale: matches `railway-type-definitions` precedent and prevents importing invalid geometries. Alternative considered: allowing AREA on zip_line/goods — rejected, wiki explicitly forbids.

### D2: Station modeled as POI
`aerialway_station` gets `{Name, NameAlt}`, `POI`, `GROUP routingPOI` — same pattern as `railway_station` and `amenity_ferry_terminal`. Rationale: stations are named, searchable places; POI indexing makes them findable in location search. Alternative: plain NODE without POI — rejected, would make 36k stations unsearchable.

### D3: Pylon without name/POI
`aerialway_pylon` = plain NODE, no features. Rationale: pylons are unnamed infrastructure markers, like `railway_switch`/`railway_signal`. Alternative: POI — rejected, would clutter search results with 144k unnamed objects.

### D4: Way rendering style
`zip_line` and `goods` render like `aerialway_drag_lift`: thin cable line + darker dashed carrier overlay, distinct dash pattern per type. Rationale: consistent visual language; dash distinguishes type at close zoom. Alternative: single solid line — rejected, loses type distinction.

### D5: Node symbols
New SYMBOL definitions in `include/aerialway.oss` using the primitive system (RECTANGLE/CIRCLE/POLYGON), following `include/railway.oss` patterns. Station symbol similar to `railway_station` (square with inner circle); pylon a T-shape (vertical mast + cross-arm at top) evoking a cable-support tower, distinct from `power_tower` (circle + X). Rendered via `NODE.ICON` at close zoom (pylon at veryClose), station also gets `NODE.TEXT` label.

### D6: ORDER WAYS registration
Add `aerialway_zip_line, aerialway_goods` to the existing aerialway group in `standard.oss`, `winter-sports.oss`, `cycle.oss`, `public-transport.oss`. Rationale: keeps all aerialway ways in one z-order band above roads. Alternative: separate group — rejected, no reason to interleave.

## Risks / Trade-offs

- [Style duplication across 4 stylesheets] → New types added to `include/aerialway.oss` (single source for 3 stylesheets); `public-transport.oss` keeps its inline copy per existing convention
- [`aerialway=yes` (911 uses) and other undocumented values not covered] → Out of scope: not documented on wiki, no concrete type definition to follow
- [Station AREA rendering] → No dedicated area fill defined; station areas render via building/area styles if tagged as buildings, node symbol covers the common case
