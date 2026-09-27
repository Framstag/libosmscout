# Design — new-office-types-and-styles

## Context

`map.ost` defines import-time types via the OST DSL (`TYPE name = ELEMENT (condition) {features} ADDRESS GROUP ...`). Type matching is first-match in definition order (`TypeConfig::GetNodeType` / `GetWayAreaType` iterate types in order), so `_building` variants must precede their generic counterpart. Rendering is driven by `include/office.oss` (STYLE rules keyed by `GROUP` and `TYPE`).

## Research

- OSM wiki [Key:office](https://wiki.openstreetmap.org/wiki/Key:office): key used on node + area (no way). 92 documented values.
- taginfo `key/values?key=office` (data_until 2026-08-28): 50 values with fraction >= 0.0005 (0.05%) that are documented and not discouraged.
- Element types verified per value via taginfo `tag/wiki_pages?key=office&value=<v>` (English page): all 50 values are `on_node: true, on_way: false, on_area: true`.

### Included values (50, alphabetical)

accountant, advertising_agency, architect, association, charity, company, construction_company, consulting, cooperative, courier, coworking, diplomatic, educational_institution, employment_agency, energy_supplier, engineer, estate_agent, financial, financial_advisor, forestry, foundation, government, graphic_design, guide, insurance, it, lawyer, logistics, moving_company, newspaper, ngo, notary, physician, political_party, property_management, publisher, quango, religion, research, security, surveyor, tax_advisor, telecommunication, therapist, translator, transport, travel_agent, union, university, water_utility

### Excluded values

| Value | fraction | Reason |
|-------|----------|--------|
| `yes` | 0.0681 | generic catch-all, covered by `office` type |
| `administrative` | 0.0024 | deprecated, use `office=government` |
| `vacant` | 0.0009 | no wiki page (undocumented) |
| `camping` | 0.0008 | discouraged ("consider a more descriptive tag") |
| `parish` | 0.0007 | discouraged ("use office=religion instead") |
| `taxi` | 0.0005 | no wiki page (undocumented) |
| `medical` | 0.0005 | discouraged ("consider more specific tags") |

## Changes

### 1. `stylesheets/map.ost` — new types

Insert 100 types (50 values × base + `_building` variant) in the Office section, before the generic `office_building`/`office` types. Each pair follows the shop pattern:

```
TYPE office_government_building
  = AREA ("office"=="government" AND EXISTS "building" AND !("building" IN ["no","false","0"]))
    {Name, NameAlt, OpeningHours, Phone, Website}
    ADDRESS
    GROUP office, building, routingPOI

TYPE office_government
  = NODE AREA ("office"=="government")
    {Name, NameAlt, OpeningHours, Phone, Website}
    ADDRESS
    GROUP office, routingPOI
```

- `ADDRESS` (no `POI`) matches the existing `office_building`/`office` types.
- `_building` variant first (AREA-only) so building areas resolve to it; base type (NODE AREA) catches nodes and non-building areas.
- Values sorted alphabetically; section header documents the wiki/taginfo source and the 0.05% threshold.

### 2. `stylesheets/include/office.oss` — style coverage and symbols

Base coverage via existing rules:

- `office_<v>_building` → `GROUP office, building` → `IF _building` block (area fill + border) and building label block.
- `office_<v>` → `GROUP office` → `NODE.TEXT` and `NODE.ICON` blocks at `veryClose` zoom.

Distinct symbols for the most important office types (by taginfo usage, all >= 0.5% plus selected lower-usage types with obvious pictograms), following the amenity symbol pattern (primitives: RECTANGLE/CIRCLE/POLYGON, office color family, white cutouts):

| Symbol | Type(s) | Pictogram |
|--------|---------|-----------|
| `office_government` | government | classical building: pediment, columns, base |
| `office_estate_agent` | estate_agent | house |
| `office_insurance` | insurance | umbrella |
| `office_lawyer` | lawyer | scales of justice |
| `office_educational_institution` | educational_institution, university | graduation cap |
| `office_telecommunication` | telecommunication | signal bars |
| `office_it` | it | computer monitor |
| `office_accountant` | accountant | calculator |
| `office_diplomatic` | diplomatic | flag |
| `office_employment_agency` | employment_agency | briefcase |
| `office_research` | research | erlenmeyer flask with liquid |
| `office_architect` | architect | drafting compass |
| `office_tax_advisor` | tax_advisor | percent sign |
| `office_financial` | financial | coin stack |
| `office_logistics` | logistics | box |
| `office_advertising_agency` | advertising_agency | megaphone |
| `office_notary` | notary | document with seal |
| `office_energy_supplier` | energy_supplier | lightning bolt |
| `office_security` | security | shield |
| `office_newspaper` | newspaper | newspaper |
| `office_water_utility` | water_utility | water drop |
| `office_construction_company` | construction_company | crane |
| `office_forestry` | forestry | tree |
| `office_charity` | charity | heart |
| `office_physician` | physician | medical cross |
| `office_publisher` | publisher | book |
| `office_translator` | translator | speech bubble |
| `office_courier` | courier | package |
| `office_travel_agent` | travel_agent | paper plane with folded lower half |

Icon rules: `[MAG closer-]` block with `[TYPE office_<v>, office_<v>_building] { NODE.ICON; AREA.ICON; }` per type, placed after the generic `GROUP office` NODE.ICON rule so the specific rules win. `office_university` reuses the `office_educational_institution` symbol. Remaining office types keep the generic `office` symbol.

## Bug fix: SVG symbol border width

While comparing the SVG and PNG symbol previews (SymbolsAll), borders were missing in the SVG output (e.g. `office_accountant` calculator frame, `office_architect` compass circle). Root cause: `SymbolRendererSVG::SetBorder` ignored the `screenMmInPixel` parameter and emitted the raw mm border width as SVG `stroke-width` (0.15), while the Cairo/Qt/Skia renderers convert mm to pixels (`GetWidth() * screenMmInPixel`, ~23px at the preview scale).

Fix: `libosmscout-map-svg/src/osmscoutmapsvg/SymbolRendererSVG.cpp` — `strokeWidth = borderStyle->GetWidth() * screenMmInPixel;`. Added regression test `SetBorder converts mm width to pixels via screenMmInPixel` in `Tests/src/SymbolRendererSVGTest.cpp`.

## Validation

- `OSTAndOSSTest --warning-as-error stylesheets/map.ost stylesheets/<style>.oss` (ctest `CheckStyleSheet-*`) — all 7 stylesheets pass; confirms OST parses, OSS loads, no warnings (e.g. types without style).
- Full `ctest` suite: only pre-existing `LocationLookupTest`/`WaterIndexTest` failures due to missing `libprotobuf.so.35.1.0` shared library in the environment (unrelated to this change).
