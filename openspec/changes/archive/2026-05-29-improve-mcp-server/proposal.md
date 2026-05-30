## Why

MCPServer has only one source file with all logic inline. LocationDescription results are only partially mapped to JSON — coordDescription (basic), atNameDescription (partial). Missing: atAddressDescription, atPOIDescription, wayDescription, crossingDescription, highwayMilestoneDescription. No tool abstraction per handler.

## What Changes

- Extract LocationDescription → JSON mapping into `LocationDescriptionMapper` (header + source)
- Fully map all 7 description types: coord, atName, atAddress, atPOI, way, crossing, highwayMilestone
- Extract each tool (`version`, `locationDescription`) into own header/source pair
- MCPServer.cpp becomes thin dispatch — no inline JSON construction for tools
- Update `meson.build` with new source files
- Create `CMakeLists.txt` for MCPServer (currently missing cmake support)
- Update `tools.json` output schema to reflect complete mapping

## Capabilities

### New Capabilities
- `location-description-mapper`: Map all 7 LocationDescription types to structured JSON (structuredContent + content text)
- `version-tool`: Version tool handler (header + source)
- `location-description-tool`: Location description tool handler (header + source)

### Modified Capabilities
<!-- No existing specs modified -->

## Impact

- `MCPServer/src/MCPServer.cpp` — thin dispatch, include tool headers instead of inline logic
- New files: `MCPServer/src/LocationDescriptionMapper.h`, `.cpp`, `MCPServer/src/VersionTool.h`, `.cpp`, `MCPServer/src/LocationDescriptionTool.h`, `.cpp`
- `MCPServer/meson.build` — add new source files
- New `MCPServer/CMakeLists.txt` — cmake build support
- `MCPServer/data/tools.json` — update `locationDescription` output schema with all description types
- `MCPServer/data/openapi.json` — update with full response schemas