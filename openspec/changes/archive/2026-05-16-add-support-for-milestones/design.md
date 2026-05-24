## Context

OSM `highway=milestone` nodes carry distance info (`distance=*`), reference (`ref=*`), carriageway ref (`carriageway_ref=*`), and marker shape (`marker=*`). libosmscout's feature system has no feature that captures these tags. The `HighwayMilestoneFeature` captures this data, leaving room for future `WaterwayMilestoneFeature` and `RailwayMilestoneFeature`.

Distance is stored as `uint32_t` meters (fixed-point decimals truncated). OSM `distance` tag typically uses km. Values like `"45 + 5"` or `"35.0 mi"` are rejected with warning — only simple decimal numbers with "." separator and implicit meter unit are accepted. Floating point not used due to persistence framework constraints.

## Goals / Non-Goals

**Goals:**
- Value holder storing OSM tags from `highway=milestone` nodes
- Serialization (Read/Write) for the feature value
- OSM tag parsing in `Parse()` method
- Registration in `TypeConfig` so OST stylesheets can reference it

**Non-Goals:**
- No rendering or labeling logic (handled by stylesheets)
- No complex OSM distance format support (`"45 + 5"`, `"35.0 mi"`) — rejected with warning
- No indexing changes — milestones not indexed as addresses or POIs by default

## Decisions

### Decision 1: Feature named `HighwayMilestoneFeature` (not `MilestoneFeature`)
- **Chosen**: Use `HighwayMilestoneFeature` as the class name, feature name "HighwayMilestone".
- **Rationale**: OSM also has `waterway=milestone` and `railway=milestone`. Using `HighwayMilestoneFeature` avoids naming collisions and leaves room for sibling features.

### Decision 2: Distance stored as `uint32_t` (meters), not string or float
- **Chosen**: Store `distance` as `uint32_t` meters. `ref`, `carriageway_ref`, `marker` as `std::string`.
- **Rationale**: Persistence framework (`FileScanner`/`FileWriter`) supports `uint32_t` via `ReadUInt32Number()`/`WriteNumber()`. No floating point support. `uint32_t` covers 0 to ~4,000 km, enough for any highway.
- **Alternative considered**: `uint16_t` — too small (max 65km). `double` — not supported by persistence layer.

### Decision 3: Only simple decimal accepted for distance, warn on other formats
- **Chosen**: Parse tag value with `StringToNumber()` using "." decimal separator. Implicit unit: meters. Other formats (`"35.0 mi"`, `"45 + 5"`, comma separator) log warning and skip.
- **Rationale**: Avoids fragile format guessing. OSM `distance` convention assumes km, but we store meters for consistency with libosmscout's metric-internal convention.
- **Alternative considered**: Accept km then multiply by 1000 — rejected because `"45 + 5"` ambiguity makes reliable parsing impossible.

### Decision 4: Only allocate value if both distance AND ref are set
- **Chosen**: `Parse()` checks both `distance` and `ref` tags. If either missing, no value allocated.
- **Rationale**: `ref` is the route identifier on the milestone. A milestone without both isn't useful.

### Decision 5: Pattern follows `EleFeature` (numeric distance + string metadata)
- **Chosen**: Model after `EleFeature` — `uint32_t` distance with `ReadUInt32Number()`/`WriteNumber()` serialization. Ref/carriageway_ref/marker use string Read/Write.
- **Rationale**: `EleFeature` stores `int16_t` elevation, similar single-numeric-plus-support pattern. `uint32_t` covers 0 to ~4,000 km with integer precision.

### Decision 6: Only applies to NODE type objects
- **Chosen**: `Parse()` returns early if not node type.
- **Rationale**: `highway=milestone` is node-only per OSM wiki.

### Decision 7: Add `HighwayMilestoneDescriptionProcessor` for DumpData tool
- **Chosen**: Add a `HighwayMilestoneDescriptionProcessor` following the pattern of other feature description processors (e.g., `ContactDescriptionProcessor`). Include section entries for distance, ref, carriageway_ref, and marker.
- **Rationale**: The DumpData tool uses `DescriptionService` to render human-readable feature info. Without a processor, milestone data is invisible in dumps.
## Risks / Trade-offs

- **String storage for ref/marker fields** → Risk minimal; milestone count per dataset is small
- **Rejected distance formats** → Nodes with `"45 + 5"` or `"35.0 mi"` get no distance value. Mitigation: log warning with tag value so users can fix source data.
- **Missing `distance` or `ref` tag** → Parse returns silently, no value allocated — graceful degradation