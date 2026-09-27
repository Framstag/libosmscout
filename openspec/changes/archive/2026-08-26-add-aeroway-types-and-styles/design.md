# Design: Add missing aeroway type and style definitions

## Context

See proposal.md — Why. The stylesheet system: `stylesheets/map.ost` defines import-time `TYPE` definitions (first match wins, so type order matters); `stylesheets/*.oss` define style rules and reference types by name; `stylesheets/include/*.oss` are style modules included by `standard.oss` via `MODULE` directives. `aeroway.oss` is the aeroway style module. Symbols are defined in the `CONST`/`SYMBOL` section of `.oss` files in a 2.25-unit coordinate space; the renderer centers each symbol's bounding box and flips the y-axis (symbol y=-1.125 renders at the top of the image). `standard.oss` orders way rendering via `ORDER WAYS`; `aeroway_runway` and `aeroway_taxiway` currently sit in the railway group.

## Goals / Non-Goals

**Goals:**
- Cover all documented `aeroway=*` values with usage >= 0.05% that are not discouraged, matching the coverage pattern of `railway=*`, `historic=*`, `amenity=*`, etc.
- Keep visual consistency with existing aeroway styles (runway/taxiway color, terminal/apron fills).
- Avoid regressions: new types must not shadow generic `building` rendering for building-like features.

**Non-Goals:**
- No C++ code changes, no database format changes, no new dependencies.
- No styles for node-only infrastructure without obvious general-map visualisation beyond a symbol (e.g. `threshold`, `holding_position` get symbols but no text labels unless named).
- No support for discouraged/undocumented values (`marking`, `aerodrome_marking`, `communication`, `shelter`) or values below 0.05% usage (`control_center`, `highway_strip`, `spaceport`, `launchpad`, `launch_complex`, `arresting_gear`, `model_taxiway`).

## Decisions

### D1: Element types from the individual tag pages

Each new type's element set (NODE/WAY/AREA) comes from the individual `Tag:aeroway=*` wiki pages, which the user asked to visit for each concrete type definition. Where the main [Key:aeroway](https://wiki.openstreetmap.org/wiki/Key:aeroway) table and the tag page conflict (e.g. `hangar`: main table NODE+AREA, tag page AREA with "should not be used on nodes"), the tag page wins because it explicitly marks the element as discouraged.

*Alternative considered:* using the main Key:aeroway table as primary reference (the railway change's precedent) — rejected, the user explicitly asked to check each concrete type definition, and the tag pages carry the "should not be used" markers.

### D2: Type placement and ordering

New types are inserted in the air transport section of `map.ost` after `aeroway_gate`, before the "Landuses" section. First-match-wins means types defined earlier shadow later ones; the new `aeroway=*` values are disjoint from existing values, so ordering within the block is not semantically critical. Grouping: node types, node-way types, node-area types, way types.

### D3: Building-like types get building styles

`aeroway_hangar`, `aeroway_tower`, and `aeroway_fuel` accept AREA and would shadow the generic `building` type (defined later in `map.ost`) for areas that also carry `building=yes`. To prevent losing building rendering, all three get AREA styles in a `_building` section of `aeroway.oss`, mirroring the `railway.oss` `_building` pattern (`railway_station`, `railway_workshop`, `railway_signal_box`).

*Alternative considered:* omitting AREA from these types — rejected, the wiki documents AREA usage.

### D4: Way types reuse the runway color

`aeroway_taxilane`, `aeroway_stopway`, and `aeroway_model_runway` are paved aircraft surfaces like runway/taxiway and reuse `@aerodromeRunwayColor` with slightly narrower display widths. `aeroway_jet_bridge` is a passenger connector and renders as a thin line in the same color. All four are added to the `ORDER WAYS` aeroway group in `standard.oss` (next to `aeroway_runway`, `aeroway_taxiway`) so they draw in the correct z-order.

### D5: New symbols for node types

Nine new symbols are defined in `aeroway.oss` using `@aerowaySymbolColor`, following the existing `aeroway_gate` symbol convention: `aeroway_windsock` (pole + cone), `aeroway_heliport` (H in circle), `aeroway_fuel` (fuel pump, mirroring `shop_fuel` shape), `aeroway_tower` (control tower with windows), `aeroway_navigationaid` (beacon with rays), `aeroway_aircraft_crossing` (crossing X), `aeroway_threshold` (runway threshold bars), `aeroway_holding_position` (double bar marking), `aeroway_parking_position` (plane outline).

*Alternative considered:* reusing symbols from other modules (e.g. `shop_fuel` for fuel) — rejected, `aeroway.oss` is included by stylesheets that do not all include `shop.oss` (e.g. `cycle.oss`, `winter-sports.oss`), so cross-module symbol references would break those stylesheets.

### D6: Style zoom levels

Node symbols render at veryClose zoom (matching `aeroway_gate`); area fills at city zoom (matching existing aeroway areas); way lines at suburb/close zoom (matching runway/taxiway). Named features get text labels at close/veryClose zoom with `@labelPrioAeroway`.

### D7: Hangar is AREA-only

Per D1, `aeroway_hangar` accepts only AREA. Hangar nodes (rare in practice) are not importable; this follows the wiki tag page which explicitly says nodes "should not be used".

## Risks / Trade-offs

- [New AREA types shadow generic `building` rendering] → Mitigated by D3: `_building` section in `aeroway.oss` keeps building outlines for hangar/tower/fuel areas.
- [Symbol y-axis flip renders symbols upside down] → Mitigated by following the existing `aeroway_gate` coordinate convention and verifying via the `SymbolsAll` demo.
- [Hangar nodes not importable] → Accepted trade-off of D1/D7; wiki tag page discourages nodes.
- [`aeroway_jet_bridge` ways also tagged `highway=corridor` get aeroway type] → Accepted; jet_bridge is defined before highway types in `map.ost`, and the aeroway rendering (thin line) is appropriate for a boarding bridge.

## Migration Plan

Stylesheet-only change. No data migration, no deployment steps. Rollback: revert `map.ost`, `aeroway.oss`, and `standard.oss` changes.

## Open Questions

None.
