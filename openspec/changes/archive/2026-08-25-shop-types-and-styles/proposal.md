## Why

`stylesheets/map.ost` defines granular import-time feature types only for the "Food, beverages" group of `shop=*` values. All other shop values (clothing, health, DIY, electronics, vehicles, etc.) fall back to the generic `shop`/`shop_building` types, so the database `TypeConfig` and the renderer cannot distinguish them. The OSM wiki [Key:shop](https://wiki.openstreetmap.org/wiki/Key:shop) documents ~150 widely accepted values, and taginfo shows many of them are heavily used.

## What Changes

- Add granular `TYPE` definitions to `stylesheets/map.ost` for all shop values documented on the OSM Key:shop wiki page, plus additional values from taginfo with usage >= 0.02% that are not discouraged.
- Each value gets a plain type (`NODE AREA`) and a `_building` variant (`AREA` with `building` tag), following the existing food/beverage pattern.
- Element types (NODE/AREA) follow the OSM wiki element table: shop values are used on nodes and areas, not on ways.
- Discouraged/deprecated values (`boutique`, `fashion`, `duty_free`, `factory_outlet`, `outlet`, `no`, semicolon-combined values) are excluded; `shop=yes` stays covered by the generic `shop` type.
- Add rendering style definitions to `stylesheets/include/shop.oss` for the new types, reusing the existing shop style patterns (group-based area fill, labels, icons) and adding per-category color differentiation following the `amenity.oss` pattern.
- No changes to `standard.oss`/`cycle.oss`: both already include the shop style module.

## Capabilities

### New Capabilities
- `shop-type-definitions`: import-time feature types for all documented and relevant `shop=*` values, with correct element types, plus rendering rules so the new types are visible on maps.

### Modified Capabilities
- None. No existing spec covers shop type definitions.

## Impact

- `stylesheets/map.ost` — new `TYPE` definitions (types section, "Shops" block)
- `stylesheets/include/shop.oss` — new `CONST` colors and `STYLE` rules for shop categories
- `stylesheets/standard.oss`, `stylesheets/cycle.oss` — unchanged (already `MODULE "include/shop"`)
- Downstream: imported databases gain new types in `TypeConfig`; renderers (AGG, Cairo, OpenGL, Qt, SVG) pick up new types via the style sheets
