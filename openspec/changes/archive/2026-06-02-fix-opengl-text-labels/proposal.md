## Why

OpenGL map renderer (DrawMapOpenGL) outputs no text labels despite correct CLI parameters. This is a regression — text rendered correctly before. Missing labels severely limits map readability and makes OpenGL backend unusable for production.

## What Changes

- Add input validation and error checking to DrawMapOpenGL CLI parameter handling
- Fix root cause of missing text rendering in OpenGL backend
- Add logging/warnings when text rendering fails or is misconfigured
- Add diagnostic output for font loading and label rendering pipeline

## Capabilities

### New Capabilities
- `opengl-text-validation`: CLI parameter validation for font-related options, with clear error messages when font files are missing/unreadable or parameters are invalid
- `opengl-text-rendering-diagnostics`: Logging/diagnostic output for the text rendering pipeline — font loading success/failure, label placement decisions, shader texture upload status

### Modified Capabilities
*(None — this is a bugfix, no spec-level requirement changes)*

## Impact

- `libosmscout-map-opengl/`: Text rendering pipeline (font loading, glyph rendering, label placement)
- `Demos/`: DrawMapOpenGL CLI argument handling
- No API/ABI changes to public interfaces
- No dependency changes
