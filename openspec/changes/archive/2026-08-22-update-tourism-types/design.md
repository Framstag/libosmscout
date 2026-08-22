# Design — update-tourism-types

## Context

`map.ost` defines import-time types via the OST DSL (`TYPE name = ELEMENT (condition) {features} ADDRESS POI GROUP ...`). Type matching is first-match in definition order, so `_building` variants must precede their generic counterpart. Rendering is driven by `include/tourism.oss` (STYLE rules keyed by `GROUP` and `TYPE`).

## Changes

### 1. `stylesheets/map.ost` — new types

Insert following existing conventions (feature set, `ADDRESS`/`POI`, `GROUP`, order: `_building` before generic):

```
TYPE tourism_camp_pitch
  = NODE AREA ("tourism"=="camp_pitch")
    {Name, NameAlt}
    ADDRESS POI
    GROUP tourism, routingPOI

TYPE tourism_trail_riding_station
  = NODE AREA ("tourism"=="trail_riding_station")
    {Name, NameAlt, OpeningHours, Phone, Website}
    ADDRESS POI
    GROUP tourism, routingPOI
```

Accommodation group (near `tourism_alpine_hut` / after `tourism_motel`), with building variants first:

```
TYPE tourism_apartment_building
  = AREA ("tourism"=="apartment" AND EXISTS "building" AND !("building" IN ["no","false","0"]))
    {Name, NameAlt, OpeningHours, Phone, Website}
    ADDRESS POI
    GROUP tourism, building, routingPOI

TYPE tourism_apartment
  = NODE AREA ("tourism"=="apartment")
    {Name, NameAlt, OpeningHours, Phone, Website}
    ADDRESS POI
    GROUP tourism, routingPOI

TYPE tourism_wilderness_hut_building  // mirror tourism_alpine_hut_building: ADDRESS (no POI), GROUP tourism, building
TYPE tourism_wilderness_hut           // ADDRESS, GROUP tourism

TYPE tourism_gallery_building         // mirror tourism_museum_building
TYPE tourism_gallery                  // NODE AREA, ADDRESS POI, GROUP tourism, routingPOI
```

Rationale per type (features/groups follow the closest existing sibling):

| New type | Modeled on | Notes |
|----------|-----------|-------|
| `tourism_apartment` | `tourism_hotel` | accommodation, `routingPOI` |
| `tourism_wilderness_hut` | `tourism_alpine_hut` | unserviced hut; `ADDRESS` without `POI`, no `routingPOI` |
| `tourism_gallery` | `tourism_museum` | `routingPOI` |
| `tourism_camp_pitch` | `tourism_camp_site` | `{Name, NameAlt}` only — pitches often unnamed |
| `tourism_trail_riding_station` | `tourism_guest_house` | accommodation for riders/horses |

### 2. `stylesheets/include/tourism.oss` — area fill

Extend the `[MAG detail-]` TYPE list (currently museum/camp_site etc.) with:

```
tourism_apartment,
tourism_camp_pitch,
tourism_gallery,
tourism_trail_riding_station,
tourism_wilderness_hut
```

`_building` variants need no new rules — the existing `[GROUP tourism, building]` fill/border/label rules apply. Node icons/labels: existing `[MAG veryClose-]` `GROUP tourism` rules apply to all new types.

No other `.oss` files reference tourism types (checked: only `standard.oss` includes `include/tourism.oss`; no type lists elsewhere).

## Alternatives considered

### A. Rely on generic `tourism` catch-all only (no new types)
- **Chosen?** No.
- **Rationale:** All values are already imported as the generic `tourism` type (`EXISTS "tourism"`), but without per-value features (OpeningHours/Phone/Website), `routingPOI` classification, building-area handling, or area rendering. The existing convention (hotel, museum, camp_site, … all have dedicated types) favors specific types.
- **Risk:** Type registry growth is negligible (8 new types of ~800).

### B. Add `_building` variants for all 5 new types
- **Chosen?** No — only `apartment`, `gallery`, `wilderness_hut` (values that are themselves buildings per wiki; wiki for `wilderness_hut` explicitly recommends `building=*`). `camp_pitch` and `trail_riding_station` are not buildings.
- **Rationale:** matches existing convention (`tourism_hotel_building` etc.); avoids pointless type duplication.

### C. Also add explicit `tourism_yes` / information subtypes (`information=board`, …)
- **Chosen?** No.
- **Rationale:** `tourism=yes` already gets full generic treatment (`ADDRESS POI`, `GROUP tourism, routingPOI`); information subtypes are a separate `Key:information` concern already covered by `tourism_information`/`tourism_information_building`.

## Risk assessment

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| First-match order wrong → building areas typed as generic instead of `_building` | Low | `_building` blocks placed directly above generic blocks; mirrors existing pattern |
| OST syntax error | Low | Types follow existing verbatim patterns; validated by import test |
| Style list typo (type name mismatch) → missing fill | Low | Same names as `.ost`; DumpData/StyleConfig load test catches |
| Discouraged value slips in | Low | Explicit negative test in spec |

## Validation

- Build `Import` tool, run `Import` against a small test extract containing the new tags; verify `TypeConfig` dump contains the new types.
- `DumpData` on imported region shows types; render with `standard.oss` to check fill.
- `openspec validate update-tourism-types --type change` stays green.
