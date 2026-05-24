## Context

`HighwayMilestoneFeature` was recently added to import OSM `highway=milestone` node data. The OSM `distance` tag on these nodes uses kilometers (e.g., `distance=35` means 35 km). However, the implementation treats the parsed value as meters — no unit conversion occurs. Internally, the distance is stored as `uint32_t` meters (consistent with other libosmscout features). Two display paths (`GetLabel()` and `HighwayMilestoneDescriptionProcessor`) also output raw meters instead of kilometers.

Fix scope: 3 code paths in 2 files, plus spec update.

## Goals / Non-Goals

**Goals:**
- OSM `distance` tag parsed as km, converted to meters for internal storage (×1000)
- `GetLabel()` outputs distance in kilometers with "km" suffix
- `HighwayMilestoneDescriptionProcessor` outputs distance in kilometers
- Existing uint32_t storage format unchanged — only the value semantics

**Non-Goals:**
- No rendering or labeling changes beyond GetLabel
- No database format migration — re-import is required and acceptable
- No sub-km decimal display in km output (integer division sufficient for OSM data)

## Decisions

### Decision 1: Multiply parsed value by 1000 in `Parse()`
- **Chosen**: `value->SetDistance(static_cast<uint32_t>(d * 1000.0))` in `HighwayMilestoneFeature::Parse()`
- **Rationale**: OSM `distance` tag is km. Internal storage is meters. Simple multiplication.
- **Overflow safe**: OSM max ~10,000 km → 10,000,000 m. `uint32_t` max is ~4.29B.
- **Alternative considered**: Parse as km, store as km with unit flag — rejected, breaks existing storage contract.

### Decision 2: Integer km display in `GetLabel()`
- **Chosen**: `NumberToString(distance / 1000, locale)` + "km" 
- **Rationale**: Integer division is consistent with original spec's truncation philosophy. OSM `highway=milestone` values are virtually always whole km.
- **Alternative considered**: Float display with one decimal — rejected; unnecessary complexity, no OSM data requires it.

### Decision 3: Integer km display in `HighwayMilestoneDescriptionProcessor`
- **Chosen**: `std::to_string(value->GetDistance() / 1000)` for the distance entry value
- **Rationale**: Consistent with GetLabel output format.
- **Note**: The description entry label key stays "Distance". Only the value changes from meters to km.

## Risks / Trade-offs

- **Existing databases broken** → Re-import required. Acceptable — milestone feature is new, no production data yet.
- **Sub-km precision loss** → Integer km display truncates remainder. Mitigation: OSM data uses whole km values; if needed later, switch to `distance / 1000.0` and format with decimal.
- **Integer division vs float** → `distance / 1000` truncates. If `distance=1999` (meaning 1.999 km), display shows "1 km". Acceptable — OSM values are whole km.

## Sequence

```
OSM node (distance=35)
  │
  ▼
HighwayMilestoneFeature::Parse()
  │  StringToNumber("35") → d=35.0
  │  d * 1000 → 35000
  │  SetDistance(35000)
  ▼
FeatureValueBuffer (internal)
  │  distance=35000 (uint32_t meters)
  │
  ├── GetLabel()
  │     35000 / 1000 → 35
  │     "35 km"
  │
  └── DescriptionProcessor
         std::to_string(35000 / 1000) + " km" → "35 km"
         Entry: "Distance" → "35 km"
```