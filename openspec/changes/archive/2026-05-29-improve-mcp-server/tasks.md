## 1. Build Infrastructure

- [x] 1.1 Add dependency detection for nlohmann_json and httplib to `cmake/features.cmake` [specs: all] [1]
- [x] 1.2 Add MCPServer option + conditional add_subdirectory to root `CMakeLists.txt` [specs: all] [1]
- [x] 1.3 Add nlohmann_json and httplib to cmake dependency summary block [specs: all] [1]
- [x] 1.4 Create `MCPServer/CMakeLists.txt` linking OSMScout + nlohmann_json [specs: all] [1]
- [x] 1.5 Update `MCPServer/meson.build` with new source file list [specs: all] [1]

## 2. LocationDescriptionMapper

- [x] 2.1 Create `LocationDescriptionMapper.h` declaring three-layer `namespace osmscout::mcp` with attribute, struct, and payload mapper functions [specs: location-description-mapper] [1]
- [x] 2.2 Implement **Layer 1** attribute mappers: `GeoCoord`→JSON, `Distance`→JSON (`value` + `unit`), `Bearing`→JSON (`degrees` + `display`), `Place`→JSON (adminRegion, postalArea, poi, street, houseNumber — each optional) [specs: location-description-mapper] [2]
- [x] 2.3 Implement **Layer 2** struct mapper for `LocationCoordDescription` using attribute mappers [specs: location-description-mapper] [1]
- [x] 2.4 Implement **Layer 2** struct mapper for `LocationAtPlaceDescription` (shared by atName/atAddress/atPOI) — compose Place, Distance, Bearing attribute mappers + atPlace flag [specs: location-description-mapper] [1]
- [x] 2.5 Implement **Layer 2** struct mapper for `LocationWayDescription` [specs: location-description-mapper] [1]
- [x] 2.6 Implement **Layer 2** struct mapper for `LocationCrossingDescription` [specs: location-description-mapper] [2]
- [x] 2.7 Implement **Layer 2** struct mapper for `LocationHighwayMilestoneDescription` [specs: location-description-mapper] [2]
- [x] 2.8 Implement **Layer 3** payload mapper — iterate all 7 descriptions, call struct mappers, assemble `content[]` + `structuredContent` [specs: location-description-mapper] [2]

## 3. VersionTool

- [x] 3.1 Create `VersionTool.h` declaring `ToolResult` struct and `HandleVersion()` function [specs: version-tool] [1]
- [x] 3.2 Implement `VersionTool.cpp` — return version string in structuredContent + handle fallback [specs: version-tool] [1]

## 4. LocationDescriptionTool

- [x] 4.1 Create `LocationDescriptionTool.h` declaring `HandleLocationDescription()` function [specs: location-description-tool] [1]
- [x] 4.2 Implement `LocationDescriptionTool.cpp` — validate input (latitude/longitude required, numeric) [specs: location-description-tool] [1]
- [x] 4.3 Implement `HandleLocationDescription()` — call `DescribeLocation()`, invoke `LocationDescriptionMapper::ToJson()`, build response [specs: location-description-tool] [3]

## 5. Refactor MCPServer.cpp

- [x] 5.1 Replace inline `HandleMessageToolsCall` with dispatch to `VersionTool` and `LocationDescriptionTool` [specs: all] [2]
- [x] 5.2 Remove inline LocationDescription JSON construction code from MCPServer.cpp [specs: all] [1]
- [x] 5.3 Verify no httplib include leakage in tool/mapper headers [specs: all] [1]

## 6. Update Data Files

- [x] 6.1 Update `MCPServer/data/tools.json` — add full outputSchema for `locationDescription` with all 7 description types [specs: location-description-mapper] [2]
- [x] 6.2 Delete `openapi2.json` and `data/openapi.json`, remove routes from MCPServer.cpp, update README [specs: location-description-mapper] [1]

## 7. Build & Verify

- [x] 7.1 Build MCPServer with meson (verify compilation) [specs: all] [1]
- [x] 7.2 Build MCPServer with cmake (verify compilation) [specs: all] [1]
- [x] 7.3 Run smoke test: server starts, parses args, handles db-not-found correctly [specs: all] [2]