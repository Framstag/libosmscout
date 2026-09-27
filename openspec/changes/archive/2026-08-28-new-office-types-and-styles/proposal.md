## Why

`stylesheets/map.ost` defines only two generic office types (`office` and `office_building`), while the OSM wiki [Key:office](https://wiki.openstreetmap.org/wiki/Key:office) documents ~92 concrete `office=*` values. Objects carrying these tags are only imported as the generic `office` catch-all type, so they lack specific feature extraction and cannot be distinguished in the database `TypeConfig`.

Per taginfo, 50 documented values have relevant usage (>=0.05% of all `office=*` objects) and are not discouraged. Adding dedicated types for these values makes them importable, searchable, and renderable as distinct office categories.

## What Changes

- Add import-time types to `stylesheets/map.ost` for 50 documented `office=*` values with taginfo usage >= 0.05%, following the existing office conventions (feature set, `ADDRESS`, `GROUP` incl. `routingPOI`, `_building` variants for areas carrying a `building=*` tag, mirroring the shop/amenity pattern):
  - `office_<value>` (node, area) + `office_<value>_building` (area) for each value
  - Values: accountant, advertising_agency, architect, association, charity, company, construction_company, consulting, cooperative, courier, coworking, diplomatic, educational_institution, employment_agency, energy_supplier, engineer, estate_agent, financial, financial_advisor, forestry, foundation, government, graphic_design, guide, insurance, it, lawyer, logistics, moving_company, newspaper, ngo, notary, physician, political_party, property_management, publisher, quango, religion, research, security, surveyor, tax_advisor, telecommunication, therapist, translator, transport, travel_agent, union, university, water_utility
- Element types follow the OSM wiki tag pages: all 50 values are documented as node + area (no way).
- Discouraged/deprecated values (`office=administrative`, `office=camping`, `office=parish`, `office=medical`) and undocumented values (`office=vacant`, `office=taxi`) SHALL NOT get dedicated types; they remain covered by the generic `office` type. `office=yes` is the generic catch-all and needs no dedicated type.
- Style: new types are covered by the existing `GROUP office` rules in `stylesheets/include/office.oss` (building fill/border, building label, node label, node icon). Additionally, distinct symbols are defined for the most important office types (by taginfo usage) and wired via `NODE.ICON`/`AREA.ICON` rules at closer zoom, following the amenity symbol pattern.
- No file-format change: existing databases can be re-imported to pick up the new types.

## Capabilities

### New Capabilities
- `office-type-definitions`: import-time types for the documented `office=*` values with relevant usage, and their rendering.

### Modified Capabilities
<!-- none: existing specs are unrelated -->

## Impact

- `stylesheets/map.ost` — 100 new types (50 values × base + `_building` variant)
- `stylesheets/include/office.oss` — 29 distinct symbols for the most important office types + `NODE.ICON`/`AREA.ICON` rules; existing `GROUP office` rules cover the remaining types
- Imported databases: new types appear in `TypeConfig` after re-import; address search and routing POI search gain the new categories
- Renderers: areas of new types get the office building fill/border and label; nodes get the office label and icon via the existing `GROUP office` rules
