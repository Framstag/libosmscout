## Purpose

The `version-tool` capability provides an MCP tool handler that returns the libosmscout library version string. It has no input parameters and is used for client-side capability detection.

## Requirements

### Requirement: VersionTool returns library version

The `VersionTool` SHALL return the libosmscout library version when called with the `version` tool name.

#### Scenario: Successful version lookup
- **GIVEN** a valid request for tool `version`
- **WHEN** `HandleVersion()` is called
- **THEN** the result SHALL have status 200
- **AND** the body SHALL contain `jsonrpc: "2.0"` and the request `id`
- **AND** `result.structuredContent.version` SHALL contain the version string

#### Scenario: Error handling
- **GIVEN** the version string is unavailable
- **WHEN** `HandleVersion()` is called
- **THEN** the result SHALL still return a 200 status with a sensible fallback version string

### Requirement: VersionTool has no input parameters

The `VersionTool` SHALL NOT require any input parameters beyond the MCP request envelope.

#### Scenario: No arguments needed
- **GIVEN** a `tools/call` request for `version`
- **WHEN** the request has no `params.arguments`
- **THEN** the tool SHALL succeed and return a version