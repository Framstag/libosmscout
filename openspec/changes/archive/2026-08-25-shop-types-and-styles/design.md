# Design: shop-types-and-styles

## Context

See proposal.md — Why. `stylesheets/map.ost` currently defines granular import-time types only for the "Food, beverages" group of `shop=*` (24 values, each with a `_building` variant). All other shop values fall back to the generic `shop`/`shop_building` types. The rendering module `stylesheets/include/shop.oss` styles all shops generically via the `shop` group. The amenity module (`include/amenity.oss`) demonstrates the per-type styling pattern used for granular amenity types.

## Goals / Non-Goals

**Goals**
- Add granular `TYPE` definitions for all documented Key:shop values plus taginfo values >= 0.02% (not discouraged), following the existing food/beverage pattern exactly.
- Add rendering rules in `include/shop.oss` so new types are visually distinguishable.
- Keep the generic `shop`/`shop_building` types as fallback for unknown/undocumented values.

**Non-Goals**
- No changes to the import pipeline, database format, or renderer code — this is a stylesheet-only change.
- No per-value icons for every shop value: libosmscout does not ship the OSM wiki SVG icon set; symbols are drawn with the built-in primitives (RECTANGLE/CIRCLE/POLYGON) for the most recognizable shop types, and the generic shop symbol remains the fallback.
- No changes to `standard.oss`/`cycle.oss` module lists (both already include `include/shop`).

## Decisions

### Decision 1: Granular types for all documented values, not just food/beverage

**Chosen**: Extend the existing pattern to all wiki-documented shop values (~150) plus 9 taginfo-only values >= 0.02%.

**Alternatives**:
- *Keep generic fallback only*: simplest, but the task explicitly requires granular types, and granularity enables per-type rendering and POI search differentiation.
- *Granular only for values above a usage threshold*: would exclude documented wiki values like `shop=brewing_supplies` (0.01%) that are still real types; the wiki is the authoritative list of concrete type definitions.

**Rationale**: The wiki Key:shop page is the canonical list of concrete shop type definitions. Taginfo adds values the wiki table omits. Both sources are used: wiki values always, taginfo-only values when >= 0.02% and not discouraged.

### Decision 2: Element types NODE + AREA for all shop types

**Chosen**: Plain types are `NODE AREA`; `_building` variants are `AREA` with `EXISTS "building"` and `!("building" IN ["no","false","0"])`.

**Alternatives**:
- *Include WAY*: rejected — the wiki element table for Key:shop and every spot-checked tag page (mall, storage_rental, car, outpost, vacant, caravan) marks ways as "should not be used".
- *AREA only for some values*: rejected — no wiki tag page restricts any shop value to area-only; all allow nodes.

**Rationale**: Matches the wiki element table and the existing food/beverage pattern.

### Decision 3: Per-category style colors in include/shop.oss

**Chosen**: Add `CONST` colors per shop category (food, general, clothing, health, DIY, furniture, electronics, vehicles, art, stationery, other) and `[TYPE ...]`-based area fill/border rules for building variants, following the `amenity.oss` pattern. Keep the existing generic `GROUP shop` rules as fallback for labels, icons, and any type not explicitly listed.

**Alternatives**:
- *Keep single generic shop color*: simpler, but provides no visual differentiation between e.g. a supermarket and a car dealer; the task asks for style definitions where visualization is obvious.
- *Per-value colors*: 150+ color constants is unmaintainable; categories are the right granularity.

**Rationale**: Category colors follow the amenity.oss precedent (hospital, post, parking each have distinct colors), keep the file maintainable, and make the new types visibly distinct.

### Decision 4: Type naming for non-identifier values

**Chosen**: Type names replace `-` with `_` (e.g. `shop=e-cigarette` → `shop_e_cigarette`); the condition keeps the literal tag value (`"shop"=="e-cigarette"`).

**Alternatives**: *Literal hyphen in type name* — rejected, no existing type name contains a hyphen and it risks parser/consumer issues.

**Rationale**: Matches existing convention (e.g. `aerialway=t-bar` is matched by a condition with the literal value while type names use underscores).

### Decision 5: Placement and ordering in map.ost

**Chosen**: Insert new types in the existing "Shops" section, organized by wiki category with comment headers, before the generic `shop_building`/`shop` types.

**Alternatives**: *Append after generic types* — rejected: type matching is first-match-wins, so specific types must precede the generic catch-all.

**Rationale**: First-match-wins semantics require specific-before-generic ordering; category grouping mirrors the wiki and keeps the file navigable.

### Decision 6: Per-type node/area symbols for recognizable shop types

**Chosen**: Add ~64 `SYMBOL` definitions drawn with the built-in primitives (RECTANGLE/CIRCLE/POLYGON) for the most recognizable shop types (cart, loaf, scissors, car, bike, book, t-shirt, ring, flower, paw, phone, bottle, wine glass, cup, cone, gift, glasses, gas bottle, tyre, note, camera, frame, pencil, ticket, fish, apple, cheese, droplet, bed, shield, key, candle, vase, sailboat, house, balloon, washer, hanger, play button, dice, newspaper, kiosk, globe, block, TV, monitor, door, flame, heart, surfboard, skis, plane, truck, pump, lollipop, chocolate bar, cake, milk bottle, coin, bulb). Per-type `NODE.ICON`/`AREA.ICON` rules at `[MAG closer-]` assign them, following the `man_made.oss` hospital/parking pattern. The generic `shop` symbol remains the fallback for all other types.

**Alternatives**:
- *No symbols (generic rectangle only)*: simplest, but the task asks for style definitions where visualization is obvious, and recognizable icons are the obvious visualization for shops.
- *Import the OSM wiki SVG icon set*: out of scope — libosmscout symbols are defined in the style sheet with primitives, not external SVGs.

**Rationale**: Simple geometric symbols are cheap, keep the style sheet self-contained, and follow the existing symbol conventions (amenity, tourism, natural).

### Decision 7: All POI icon symbols scaled to ~2.25mm span

**Chosen**: Scale every POI icon symbol in `include/*.oss` to a ~2.25mm span (≈8.5px @ 96 DPI), per-symbol factor = 2.25/span (only grow), border widths scaled proportionally. Files: `shop.oss` (64 symbols), `railway.oss`, `waterway.oss`, `leisure.oss`, `office.oss`, `tourism.oss`, `historic.oss`, `military.oss`, `aeroway.oss`, `man_made.oss`, `natural.oss`, `roads.oss` (POI symbols only).

**Alternatives**:
- *Leave sizes as-is*: rejected — symbols rendered at 1–2mm span (≈4–7.5px @ 96 DPI), too small to see in OSMScout2.
- *Set `width`/`height` on ICON rules*: impossible — `IconStyle` width/height apply only to `name:`-based icons; the ICON rule descriptor has no size attributes for `symbol:`-based icons.
- *Renderer DPI change*: rejected — renderer-level setting, not stylesheet; OSMScout2 uses screen physical DPI.

**Rationale**: Symbol pixel size = primitive coordinate span (mm) × projection DPI (`SymbolRenderer::Render` uses fixed scaleFactor 1.0). Scaling coordinates is the only stylesheet-level control. Excluded from scaling: GROUND symbols (ground coords in meters), road/route symbols (`turn_*` lane arrows, `oneway_arrow`, `viaFerrata*` crosses, `highway_turning_cycle`), `place.oss` city markers, `route.oss` UI markers, standalone `public-transport.oss`/`railways.oss`.

## Risks / Trade-offs

- [Large type count (~320 new TYPE blocks) increases map.ost size and import type-config size] → Mitigation: types are cheap; the generic fallback still catches unknown values; blocks are mechanical and generated from the wiki/taginfo tables in the spec.
- [Per-category colors are a subjective visual choice] → Mitigation: derive all category colors from the existing `@shopColor` family (lighten/darken), so the palette stays coherent; generic rules remain as fallback.
- [First-match-wins ordering bug would shadow specific types] → Mitigation: new types inserted before `shop_building`/`shop`; verification task checks type resolution order.
- [Wiki/taginfo drift: new shop values appear after this change] → Mitigation: generic `shop` type catches unknown values; future updates can extend the granular list.

## Migration Plan

Stylesheet-only change. No data migration: existing databases keep working (generic types still match); re-import with the updated `map.ost` produces the new granular types. Rollback = revert the two stylesheet files.

## Open Questions

None.
