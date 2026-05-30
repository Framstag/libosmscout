# MCPServer

A simple MCP (Model Context Protocol) server for libosmscout. Implements enough of the MCP specification to run with `mcp-inspector` or `mcpo`.

## Usage

```
MCPServer DATA_PATH DATABASE [--host HOST] [--port PORT]
```

- `DATA_PATH` — directory with MCP JSON data files (e.g., `MCPServer/data/`)
- `DATABASE` — path to libosmscout database directory
- `--host` — HTTP host (default: `0.0.0.0`)
- `--port` — HTTP port (default: `8000`)

Requires calling from repository root to resolve data file paths.

## Testing

```bash
# mcp-inspector (streamable HTTP)
npx @modelcontextprotocol/inspector
# use "http://localhost:8000" with "streamable http" protocol

# mcpo integration for Open-WebUI
uvx mcpo --port 8888 --server-type streamable_http -- http://localhost:8000
```

## Architecture

```
MCPServer/
├── src/
│   ├── MCPServer.cpp                 — thin dispatch (parse → route → respond)
│   ├── LocationDescriptionMapper.h/.cpp — three-layer mapper
│   │   Layer 1: Attribute mappers (GeoCoord, Distance, Bearing, Place → JSON)
│   │   Layer 2: Struct mappers (each LocationDescription* type)
│   │   Layer 3: Payload mapper (content[] + structuredContent)
│   ├── VersionTool.h/.cpp            — "version" tool handler
│   └── LocationDescriptionTool.h/.cpp — "locationDescription" tool handler
├── data/                             — MCP protocol JSON files
│   ├── capabilities.json
│   ├── initialized.json
│   └── tools.json
├── CMakeLists.txt
└── meson.build
```

Adding new tools: create a `NewTool.h/.cpp` pair with a `HandleNewTool()` function returning `ToolResult`, wire it into `HandleMessageToolsCall` in `MCPServer.cpp`.