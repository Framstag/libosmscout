## Context

Current `HighwayMilestoneFeature::Parse()` reads `distance` tag value, parses as plain double via `StringToNumber()`, multiplies by 1000 (implicit km assumption), stores as `uint32_t` meters. Values with unit suffixes like `"35.0 mi"` fail parsing entirely — `StringToNumber()` rejects non-numeric content. `GetLabel()` and `DescriptionService` both divide by 1000 and append `"km"`.

OSM convention: `distance` without unit = km, `"mi"` suffix = miles, `"km"` suffix = km. The tag value format is `<number> <unit>` with optional space.

## Goals / Non-Goals

**Goals:**
- Parse `"km"` and `"mi"` unit suffixes from distance tag value
- Convert both to internal meter representation (km × 1000, mi × 1609.344)
- Default to km when no unit present (backward compatible)
- Extensible design: adding a new unit requires one entry in a table
- Maintain binary format: distance stays `uint32_t` meters, no new fields
- Update `GetLabel()` to display in km (converted from meters, for all units)
- Update description processor similarly

**Non-Goals:**
- No `"45 + 5"` combined km+meter format (explicitly excluded per OSM wiki)
- No comma-as-decimal-separator support
- No storing original unit — meter is canonical internal representation
- No changes to serialization format
- No changes to type registration or stylesheets

## Decisions

### Decision 1: Unit table with factors for extensibility

**Chosen**: Define a `static constexpr` array of unit entries, each with suffix string and meter-conversion factor. Parse searches the table for a matching suffix.

```
struct DistanceUnit {
  const char* suffix;
  double factorToMeters;  // multiply value by this to get meters
};

static constexpr DistanceUnit units[] = {
  {"km", 1000.0},
  {"mi", 1609.344}
};
```

**Rationale**: Adding a new unit (e.g. `"nmi"` for nautical miles) requires one line. Table lookup is O(n) with n < 5, negligible for per-node parsing.

**Alternative considered**: Switch on first letter of suffix — fragile (mi, m, km could conflict). Virtual dispatch — overkill for 2 units.

**Alternative considered**: `std::unordered_map` — heap allocation overhead and not `constexpr`. Array is simpler and zero-overhead.

### Decision 2: Parse by stripping suffix from end of value string

**Chosen**: After `StringToNumber()` fails on the raw string (or better: before numeric parse), check for trailing unit suffix by scanning the string from the end. If a suffix matches:
1. Extract the numeric prefix before the suffix
2. Verify numeric parse succeeds on the prefix
3. Apply conversion factor
4. If no suffix matches, fall through to existing behavior (no unit = km)

**Rationale**: Simple string ops, no regex dependency. OSM tag format is `<number><space><unit>` but we handle optional space and contiguous `"35mi"` too.

**Pseudo:**
```
auto value = trim(distanceString);
// Check for unit suffix
for (auto& unit : units) {
  if (value ends with unit.suffix) {
    auto prefix = value without suffix, trimmed
    if (!StringToNumber(prefix, d)) { warn; skip; }
    value->SetDistance(static_cast<uint32_t>(d * unit.factorToMeters));
    return; // or handle other fields
  }
}
// No unit matched → existing behavior (km default)
if (!StringToNumber(value, d)) { warn; skip; }
value->SetDistance(static_cast<uint32_t>(d * 1000.0));
```

### Decision 3: `GetLabel()` and description processor unchanged

Both already divide by 1000 and display km. Since internal representation is always meters, this is correct regardless of original unit. `10 mi` → `16093 m` → label shows `"16 km"` — acceptable because:
- Milestone label is approximate (OSM values are often rounded)
- km is the OSM standard display unit
- Changing to raw meters would break UIs expecting km

### Decision 4: Reject `"45 + 5"` via StringToNumber failure

No special handling needed — the plus sign and extra number will cause `StringToNumber()` to fail after stripping unit prefix, producing the same warning-and-skip behavior as the current implementation.

### Decision 5: Update test scenarios

Add the following scenarios to existing milestone tests:
- `distance=35.0 km` → 35000 m
- `distance=35 km` → 35000 m
- `distance=10 mi` → 16093 m (uint32_t(10 * 1609.344) = 16093)
- `distance=10.5 mi` → 16898 m (uint32_t(10.5 * 1609.344) = 16898)
- `distance=35 km` with ref → value allocated (existing)
- `distance=10 mi` with ref → value allocated (new: unit suffix should not prevent allocation)
- `distance=45 + 5` → warning, no value (existing behavior unchanged)

## Risks / Trade-offs

- **Float rounding in uint32_t cast** → `10 mi * 1609.344 = 16093.44`, truncated to 16093. Acceptable (sub-meter error for road distance).
- **"mi" substring in numeric value** → e.g. `"3.5"` won't match "mi". Safe because suffix match is exact from end.
- **Prefix collision risk** → "km" and "mi" are distinct. Future units must be checked for prefix conflicts (e.g. "m" could match within "km" — check longest suffixes first, or require suffix match to be after a space or at string end).
- **No rollback needed** — binary format unchanged. Old clients read new data as meters (same as before). New clients read old data correctly (old data stored km*1000 = meters). Fully backward compatible.