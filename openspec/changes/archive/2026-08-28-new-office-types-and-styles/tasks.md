# Tasks — new-office-types-and-styles

## 1. Type definitions in map.ost (spec: office types with relevant usage)

- [x] Add 50 `office_<value>` types (NODE AREA, `{Name, NameAlt, OpeningHours, Phone, Website}`, ADDRESS, GROUP office, routingPOI) to `stylesheets/map.ost` Office section, before the generic `office_building`/`office` types.
- [x] Add 50 `office_<value>_building` variants (AREA, building condition, GROUP office, building, routingPOI), each directly above its generic counterpart.
- [x] Values: accountant, advertising_agency, architect, association, charity, company, construction_company, consulting, cooperative, courier, coworking, diplomatic, educational_institution, employment_agency, energy_supplier, engineer, estate_agent, financial, financial_advisor, forestry, foundation, government, graphic_design, guide, insurance, it, lawyer, logistics, moving_company, newspaper, ngo, notary, physician, political_party, property_management, publisher, quango, religion, research, security, surveyor, tax_advisor, telecommunication, therapist, translator, transport, travel_agent, union, university, water_utility.
- [x] Verify no discouraged/undocumented types added (spec: discouraged values): `office_administrative`, `office_camping`, `office_parish`, `office_medical`, `office_vacant`, `office_taxi`, `office_yes` must NOT exist.

## 2. Style rules (spec: rendering of new office types)

- [x] Confirm `_building` variants covered by existing `GROUP office, building` rules.
- [x] Confirm base types covered by existing `GROUP office` NODE.TEXT/NODE.ICON rules.
- [x] Add wiki reference comment to `stylesheets/include/office.oss` section header.
- [x] Add 29 distinct symbols to `stylesheets/include/office.oss` for the most important office types (government, estate_agent, insurance, lawyer, educational_institution, telecommunication, it, accountant, diplomatic, employment_agency, research, architect, tax_advisor, financial, logistics, advertising_agency, notary, energy_supplier, security, newspaper, water_utility, construction_company, forestry, charity, physician, publisher, translator, courier, travel_agent).
- [x] Add `[MAG closer-]` NODE.ICON/AREA.ICON rules per type (incl. `_building` variants); `office_university` reuses the `office_educational_institution` symbol.
- [x] Regenerate symbol overview with SymbolsAll and verify new symbols render.

## 3. Build & validation

- [x] Fix SVG symbol border width bug: `SymbolRendererSVG::SetBorder` now converts mm width to pixels via `screenMmInPixel` (was emitting raw mm as `stroke-width`, making borders invisible in SVG output).
- [x] Add regression test `SetBorder converts mm width to pixels via screenMmInPixel` to `Tests/src/SymbolRendererSVGTest.cpp`.
- [x] Run `cd build && ctest -R CheckStyleSheet --output-on-failure` — all 7 stylesheet checks pass (OST parses, OSS loads, no warnings).
- [x] Run `cd build && ctest -R SymbolRendererSVG --output-on-failure` — passes.
- [x] Run full test suite: `cd build && ctest -j 2 --output-on-failure` — only pre-existing `LocationLookupTest`/`WaterIndexTest` failures (missing `libprotobuf.so.35.1.0` in environment, unrelated).
- [x] `openspec validate new-office-types-and-styles --type change` passes.
- [x] Regenerate symbol overview with SymbolsAll; SVG/PNG previews now match (pixel diff < 0.01%).
- [ ] Optionally render an imported region with `standard.oss` and verify office rendering for new types.
