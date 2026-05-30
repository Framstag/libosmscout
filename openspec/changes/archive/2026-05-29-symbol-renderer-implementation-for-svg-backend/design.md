## Context

The SVG map backend (`libosmscout-map-svg`) renders OSM data to SVG output via `MapPainterSVG` which extends `MapPainter`. Symbol rendering (shields, markers, POI icons composed of geometric primitives) was implemented directly in `MapPainterSVG::DrawSymbol` with inline coordinate projection, primitive type dispatch, and SVG element generation.

The core library provides a `SymbolRenderer` interface (in `libosmscout-map/include/osmscoutmap/SymbolRenderer.h`) with:
- Two `Render()` methods (with/without transformer callbacks)
- Pure virtual methods: `SetFill()`, `SetBorder()`, `DrawPolygon()`, `DrawRect()`, `DrawCircle()`
- Optional virtual `BeginPrimitive()` / `EndPrimitive()` hooks (default no-op)

The Qt backend already implements this interface as `SymbolRendererQt`. The SVG backend needs an equivalent `SymbolRendererSVG` to align with the unified rendering pattern.

Constraints:
- SymbolRenderer base class handles all projection math and primitive iteration
- SVG output goes to a `std::ostream&` owned by `MapPainterSVG`
- No external SVG/graphics library — raw string formatting to stream
- Must maintain backward-compatible SVG output (identical rendering)

## Goals / Non-Goals

**Goals:**
- Create `SymbolRendererSVG` implementing `SymbolRenderer` for the SVG backend
- Refactor `MapPainterSVG::DrawSymbol` to delegate to `SymbolRendererSVG`
- Maintain identical SVG output (regression-free)
- Follow same pattern as `SymbolRendererQt` (reference implementation)
- Update CMake and Meson build files

**Non-Goals:**
- No changes to `SymbolRenderer` base class interface
- No changes to symbol data structures (`Symbol`, `DrawPrimitive`, `PolygonPrimitive`, etc.)
- No changes to other backends (Qt, AGG, Cairo, OpenGL)
- No SVG output format changes — same element structure, attributes, spacing

## Decisions

### Decision 1: Constructor takes `std::ostream&` (not stored pointer)
- **Choice**: Pass stream reference to constructor, store as reference member
- **Rationale**: Non-owning reference matches `SymbolRendererQt` which takes `QPainter*`. Reference instead of pointer eliminates null-check burden. Stream is always valid for the lifetime of the renderer (single `DrawSymbol` call).
- **Alternatives considered**:
  - `QPainter*`-style pointer: Adds null check overhead for no benefit
  - Pass stream to each draw method: Verbose, inconsistent with interface pattern

### Decision 2: Stateful fill/stroke tracking (not per-call)
- **Choice**: Track `hasFill`/`hasStroke`/`strokeWidth` state from `SetFill()`/`SetBorder()` calls, emit attributes in `WriteFillAndStroke()`
- **Rationale**: `SetFill()` and `SetBorder()` are called before each draw primitive, exactly matching the Qt backend pattern. Writing attributes inline in each draw method would duplicate the attribute formatting logic 3x.
- **Alternatives considered**:
  - Write fill/stroke attributes inline in each `Draw*()` method: More duplication, harder to maintain
  - Use a separate XML builder object: Overkill for simple attribute generation

### Decision 3: Use `polyline` not `polygon` for polygon primitives
- **Choice**: `<polyline>` element (as existing DrawSymbol did)
- **Rationale**: Existing behavior uses `<polyline>`. Symbols are filled shapes, but `<polyline>` with `fill` attribute works for SVG renderers. Changing to `<polygon>` would alter output and risk regression. SVG spec allows fill on `<polyline>` even though it's not technically closed — common practice.
- **Alternatives considered**: `<polygon>` element: Would produce identical visual but different output text

### Decision 4: Disable copy/move semantics on SymbolRendererSVG
- **Choice**: Delete copy constructor, move constructor, copy assignment, move assignment
- **Rationale**: `SymbolRenderer` base class doesn't define these, and the Qt implementation (`SymbolRendererQt`) uses `= default`. However SVG renderer holds a `std::ostream&` reference — copying would create ambiguous stream ownership. Deleting prevents misuse.
- **Alternatives considered**: Default copy/move: Would copy the stream reference, semantically unclear who owns output

### Decision 5: Color encoding matches existing SVG pattern
### Decision 5: Use Color::ToHexString() for color encoding
- **Choice**: Call `color.ToHexString()` (existing public method on `Color`)
- **Rationale**: `Color::ToHexString()` already exists in the core library and produces `#RRGGBB` / `#RRGGBBAA`. Avoids duplicating hex encoding logic.
- **Alternatives considered**: Custom static helper: would duplicate core library functionality

## Risks / Trade-offs

- [Output drift] Refactoring could subtly change SVG output (whitespace, element order, attribute formatting) → Mitigation: Compare generated SVG before and after; design explicitly keeps same element types and attribute format
- [Missing dash support] `SetBorder()` receives no cap/dash info from base class; SVG dash patterns cannot be emitted → Mitigation: Acceptable — base interface doesn't provide this data. If needed later, it's an interface-level change
- [Stream errors] SVG stream could enter error state mid-render → Mitigation: Same as existing code — no explicit error handling, matches existing design
- [Alpha channel] Color alpha not emitted in SVG attributes → Mitigation: Existing behavior also ignores alpha in fill/stroke colors. Full alpha support would require `opacity` attribute handling across all backends

## Migration Plan

Single atomic change:
1. Add `SymbolRendererSVG.h` and `SymbolRendererSVG.cpp`
2. Update build files
3. Refactor `DrawSymbol` to use `SymbolRendererSVG`
4. Build and verify SVG output matches
5. No rollback needed — trivially revertible

## Open Questions

- Should `SymbolRendererSVG` support SVG `opacity` attribute for non-solid colors? Deferred — requires base interface changes for transparency.