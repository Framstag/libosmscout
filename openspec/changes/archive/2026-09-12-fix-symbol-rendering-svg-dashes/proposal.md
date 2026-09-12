## Why

The SVG symbol renderer drops dashed borders entirely: `SetBorder()` leaves the dash branch empty, so symbols whose border style defines dashes render with a solid outline in SVG output while the Cairo and Qt backends render the dashes. Symbol rendering is therefore inconsistent across backends for the same stylesheet data.

## What Changes

- SVG symbol renderer: dashed borders emit an SVG dash pattern (`stroke-dasharray`) scaled consistently with the converted border width, so dashes appear in SVG output the same way they do in the Cairo backend.
- Solid borders keep their current output — no dash attribute is emitted.
- Test coverage for dashed and solid border output in `SymbolRendererSVGTest`.
- Update the `svg-symbol-renderer` capability spec with the dashed border requirement.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `svg-symbol-renderer`: requirement that dashed borders emit a scaled dash pattern and solid borders emit none.

## Impact

- `libosmscout-map-svg/include/osmscoutmapsvg/SymbolRendererSVG.h` — renderer state for the dash pattern
- `libosmscout-map-svg/src/osmscoutmapsvg/SymbolRendererSVG.cpp` — dash pattern emission in `SetBorder()` and the stroke writer
- `Tests/src/SymbolRendererSVGTest.cpp` — new test cases for dashed and solid borders
- `openspec/specs/svg-symbol-renderer/spec.md` — capability spec update
- No changes to other backends, stylesheets, or APIs
