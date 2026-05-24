## Context

MCPServer currently lives in a single `src/MCPServer.cpp` (385 LOC). LocationDescription → JSON mapping is partial and inline inside `HandleMessageToolsCall`. Each tool handler (`version`, `locationDescription`) is also inline in the same file. The server uses httplib for HTTP, nlohmann::json for JSON, and libosmscout's LocationDescriptionService for geo lookups.

Files involved:
- `MCPServer/src/MCPServer.cpp` — all logic
- `MCPServer/meson.build` — only meson support, no cmake
- `MCPServer/data/tools.json` — MCP tool list + output schemas
- `MCPServer/data/openapi.json` — OpenAPI spec for mcpo integration
- `libosmscout/include/osmscout/location/LocationDescriptionService.h` — 7 description types
- `libosmscout/include/osmscout/location/Location.h` — Place, AdminRegion, POI, Location, Address

## Goals / Non-Goals

**Goals:**
- Extract LocationDescription→JSON mapping into dedicated `LocationDescriptionMapper` (namespace with free functions)
- Fully map all 7 description types: coord, atName, atAddress, atPOI, way, crossing, highwayMilestone
- Extract each tool into own header/source: `VersionTool`, `LocationDescriptionTool`
- `MCPServer.cpp` becomes thin dispatch — parse method, route to tool, send response
- Update `meson.build` and add `CMakeLists.txt`
- Update `tools.json` and `openapi.json` with complete output schemas

**Non-Goals:**
- No changes to libosmscout core types or LocationDescriptionService
- No new tools beyond what exists (version, locationDescription)
- No changes to MCP protocol handling (initialize, initialized, ping, tools/list)
- No unit tests for this change (Catch2 setup for MCPServer out of scope)

## Decisions

### 1. LocationDescriptionMapper as namespace with free functions

**Decision**: Use `namespace osmscout::mcp { ... }` with free functions instead of a class.

**Rationale**: Pure mapping with no state. Free functions are simpler, testable, and match the "transformer" pattern. No constructor, no member variables needed.

```cpp
namespace osmscout::mcp {
  // === Layer 1: Attribute mappers (reusable across types) ===
  nlohmann::json ToJson(const GeoCoord& coord);
  nlohmann::json ToJson(const Distance& distance);      // → {"value": 123.4, "unit": "m"}
  nlohmann::json ToJson(const Bearing& bearing);        // → {"degrees": 45.0, "display": "NE"}
  nlohmann::json ToJson(const Place& place);            // → all Place fields, optional

  // === Layer 2: Struct mappers (compose attribute mappers) ===
  nlohmann::json ToJson(const LocationCoordDescription& desc);
  nlohmann::json ToJson(const LocationAtPlaceDescription& desc);    // shared by atName/atAddress/atPOI
  nlohmann::json ToJson(const LocationWayDescription& desc);
  nlohmann::json ToJson(const LocationCrossingDescription& desc);
  nlohmann::json ToJson(const LocationHighwayMilestoneDescription& desc);

  // === Layer 3: Payload mapper (assembles final result) ===
  nlohmann::json ToJsonContent(const LocationDescription& desc);           // content[] array entries
  nlohmann::json ToJsonStructured(const LocationDescription& desc);        // structuredContent object
  void ToJson(const LocationDescription& desc, nlohmann::json& content, nlohmann::json& structured);
}
```

#### Layer 1: Attribute mappers

Individual atomic type converters:
- `GeoCoord` → `{"latitude": 48.1, "longitude": 11.5}`
- `Distance` → `{"value": 45.2, "unit": "m"}` — use `AsMeter()` for value
- `Bearing` → `{"degrees": 90.0, "display": "E"}` — use `DisplayString()` for display
- `Place` → all optional sub-fields (adminRegion, postalArea, poi, street, houseNumber) — use existing `GetDisplayString()` for individual attributes where appropriate, but NOT for the whole Place struct (would collide with structured JSON)

#### Layer 2: Struct mappers

Compose attribute mappers for each `LocationDescription*` type. `LocationAtPlaceDescription` mapper is shared across atName/atAddress/atPOI since they share the same fields.

#### Layer 3: Payload mapper

Assembles the final `content[]` and `structuredContent` from all present description types. Iterates each optional description and delegates to Layer 2.

Alternatives considered:
- **Class with static methods**: Extra ceremony for no benefit
- **Inline in tool**: Defeats separation goal

### 2. Tool handlers return result struct, not httplib::Response&

**Decision**: Each tool handler returns a `ToolResult` struct. `MCPServer.cpp` converts to HTTP response.

```cpp
struct ToolResult {
  int status = httplib::StatusCode::OK_200;
  nlohmann::json body;
};
```

**Rationale**: Keeps httplib dependency out of tool files. Tools are testable without an HTTP server. `MCPServer.cpp` owns HTTP concerns.

Alternatives considered:
- **Pass httplib::Response& into tools**: Couples tools to httplib, harder to test
- **Return string body only**: Loses status code flexibility for error responses

### 3. Each description type gets structured JSON + text content

**Decision**: Map each description type to both `structuredContent` (typed fields) and `content` (human-readable text).

Current code only maps coordDescription and atNameDescription. Full mapping:

| Description Type | structuredContent key | Key fields |
|---|---|---|
| LocationCoordDescription | coordinateDescription | latitude, longitude, displayText |
| LocationAtPlaceDescription (atName) | atNameDescription | location, adminRegion, postalArea, poi, street, houseNumber, distanceInMeter, bearing, atPlace |
| LocationAtPlaceDescription (atAddress) | atAddressDescription | (same structure as atName) |
| LocationAtPlaceDescription (atPOI) | atPOIDescription | (same structure as atName) |
| LocationWayDescription | wayDescription | way, distanceInMeter |
| LocationCrossingDescription | crossingDescription | crossingLatitude, crossingLongitude, ways[], distanceInMeter, bearing, atPlace |
| LocationHighwayMilestoneDescription | highwayMilestoneDescription | milestoneDistance, milestoneRef, carriagewayRef, distanceInMeter, bearing, atPlace |

### 4. Directory/namespace layout

```
MCPServer/src/
  MCPServer.cpp              — thin dispatch
  VersionTool.h              — ToolResult struct + HandleVersion()
  VersionTool.cpp
  LocationDescriptionTool.h  — HandleLocationDescription()
  LocationDescriptionTool.cpp
  LocationDescriptionMapper.h  — ToJson() free functions
  LocationDescriptionMapper.cpp
```

All new code in `namespace osmscout::mcp { ... }`.

### 5. CMakeLists.txt for MCPServer

Dependencies detected in `cmake/features.cmake`:
- `find_package(nlohmann_json QUIET)`
- `check_include_file_cxx(httplib.h)` → `HAVE_HTTPLIB`

Root `CMakeLists.txt` checks both before `add_subdirectory(MCPServer)`.
If either missing: MCPServer set OFF with explanatory status message.
MCPServer/CMakeLists.txt follows existing project pattern (see DumpData).
The dependency summary block in root CMakeLists.txt lists both as new entries.

## Sequence Diagram

```
Client                  MCPServer.cpp               LocationDescriptionTool       LocationDescriptionMapper         LocationDescriptionService
  │                           │                              │                            │                              │
  │  POST / (tools/call)      │                              │                            │                              │
  │──────────────────────────>│                              │                            │                              │
  │                           │                              │                            │                              │
  │                           │  Parse JSON, extract         │                            │                              │
  │                           │  method + params             │                            │                              │
  │                           │                              │                            │                              │
  │                           │  tools/call +                │                            │                              │
  │                           │  locationDescription         │                            │                              │
  │                           │─────────────────────────────>│                            │                              │
  │                           │                              │                            │                              │
  │                           │                              │  DescribeLocation(lat,lng)  │                              │
  │                           │                              │─────────────────────────────────────────────────────────>│
  │                           │                              │                            │                              │
  │                           │                              │  LocationDescription       │                              │
  │                           │                              │<─────────────────────────────────────────────────────────│
  │                           │                              │                            │                              │
  │                           │                              │  ToJson(desc)              │                              │
  │                           │                              │───────────────────────────>│                              │
  │                           │                              │                            │                              │
  │                           │                              │  content + structured JSON │                              │
  │                           │                              │<───────────────────────────│                              │
  │                           │                              │                            │                              │
  │                           │  ToolResult{200, body}       │                            │                              │
  │                           │<─────────────────────────────│                            │                              │
  │                           │                              │                            │                              │
  │                           │  Convert to HTTP response    │                            │                              │
  │                           │  and send                    │                            │                              │
  │  HTTP 200 + JSON          │                              │                            │                              │
  │<──────────────────────────│                              │                            │                              │
```

Simpler version tool flow:
```
Client                  MCPServer.cpp               VersionTool
  │                           │                              │
  │  POST / (tools/call)      │                              │
  │──────────────────────────>│                              │
  │                           │  tools/call + version        │
  │                           │─────────────────────────────>│
  │                           │                              │
  │                           │  ToolResult{200, {"version": "..."}}
  │                           │<─────────────────────────────│
  │  HTTP 200 + JSON          │                              │
  │<──────────────────────────│                              │
```

## Risks / Trade-offs

| Risk | Mitigation |
|---|---|
| **Missing Place/FeatureValueBuffer fields in JSON output** | Place has adminRegion, postalArea, poi, location, address — all optional. Mapper checks each before adding to JSON. FeatureValueBuffer requires TypeConfig to decode; use GetDisplayString() as fallback. |
| **HighwayMilestoneDescription FeatureValueBuffer needs TypeConfig** | Requires passing TypeConfigRef to mapper, or extract display string from the Place/object. Tool receives TypeConfig from Database. |
| **Breaking changes to tools.json output schema** | Existing clients expecting old format may break. Acceptable — this is a demo server, and old format was documented as incomplete. |
| **MCPServer.cpp still handles non-tool MCP messages inline** | Non-goal. Initialize/Initialized/Ping/ToolsList are trivial and stable. Only tools with business logic are extracted. |

## Resolved Questions

- **FeatureValueBuffer for HighwayMilestoneDescription?** Use individual attribute getters and existing string representations. No TypeConfigRef needed.
- **Shared Place→JSON for atName/atAddress/atPOI?** Yes. All share `LocationAtPlaceDescription` with identical fields. One struct mapper.
- **String representations for whole structs?** No — only for individual leaf attributes. Whole struct goes through structured JSON.
- **Three-layer mapper architecture?** Yes — attribute mappers (reusable), struct mappers (compose attributes), payload mapper (assembles final result).