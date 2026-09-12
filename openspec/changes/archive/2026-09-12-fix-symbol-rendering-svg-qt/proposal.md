## Why

The SVG and Qt symbol renderers produce incorrect output for stroked symbols: polygon outlines are not closed, so the closing segment of the border is missing, and the SVG renderer emits border widths unconverted between coordinate spaces, so strokes scale incorrectly relative to the symbol canvas. This makes symbols look wrong when rendered from the same stylesheet data across backends.

This change was split out of the `further-types-styles-symbols-and-fixes` branch, which mixes these rendering fixes with large amounts of new types, styles, and icons. The fixes are self-contained and should land independently.

## What Changes

- SVG symbol renderer: border width is converted from mm to the SVG pixel coordinate space before a `stroke-width` attribute is emitted, so strokes scale consistently with the symbol canvas.
- SVG symbol renderer: polygon primitives are emitted as closed shapes, so stroked outlines include the closing segment.
- Qt symbol renderer: polygon primitives are closed before painting, so stroked outlines include the closing segment.
- Test coverage for border width conversion and closed polygon output in the SVG symbol renderer tests.
- Update the `svg-symbol-renderer` capability spec to document the border width conversion and closed polygon output requirements.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `svg-symbol-renderer`: requirement that border widths be converted from mm to pixels via the renderer's mm-per-pixel factor, and that polygon primitives be rendered as closed shapes whose stroked outline includes the closing segment.

## Impact

- `libosmscout-map-svg/src/osmscoutmapsvg/SymbolRendererSVG.cpp` — border width conversion, polygon output
- `libosmscout-map-qt/src/osmscoutmapqt/SymbolRendererQt.cpp` — polygon path closing
- `Tests/src/SymbolRendererSVGTest.cpp` — new and updated test cases
- `openspec/specs/svg-symbol-renderer/spec.md` — capability spec update
- Build systems: CMake and Meson are unaffected structurally (no new files or dependencies); existing build and test targets cover the changed sources.
