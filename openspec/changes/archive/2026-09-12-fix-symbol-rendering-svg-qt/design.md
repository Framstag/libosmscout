## Context

See proposal.md — Why. Current state: `SymbolRendererSVG::SetBorder()` receives a `screenMmInPixel` parameter but ignores it, emitting the raw border width as `stroke-width`. `SymbolRendererSVG::DrawPolygon()` emits `<polyline>`, whose stroke outline is open unless the caller re-closes the shape. `SymbolRendererQt::DrawPolygon()` builds a `QPainterPath` without closing it, so the stroked outline is open in Qt too. Tests in `Tests/src/SymbolRendererSVGTest.cpp` currently assert the buggy output.

## Goals / Non-Goals

**Goals:**
- Border widths on stroked SVG symbols scale consistently with the symbol canvas
- Stroked polygon outlines are closed in both SVG and Qt output

**Non-Goals:**
- No fix to `SymbolRendererCairo` (same class of bug exists there; intentionally out of scope, tracked separately)
- No changes to symbol rendering in AGG, OpenGL, or other backends
- No changes to stylesheets or icon data

## Decisions

### D1: Convert border width in `SetBorder()`
The `screenMmInPixel` factor SHALL be applied inside `SymbolRendererSVG::SetBorder()` when computing `stroke-width`.

- **Alternative A (chosen):** multiply `BorderStyle` width by `screenMmInPixel` at stroke emission. The parameter already exists in the `SymbolRenderer` interface and is supplied by the caller (`MapPainterSVG`); conversion at emission keeps a single source of truth and mirrors how the Qt backend already scales.
- **Alternative B:** convert in the caller before `SetBorder()`. Would double the conversion sites or require a new interface parameter; caller semantics would diverge from other backends.
- **Alternative C:** store mm width and convert per primitive. Spreads the conversion across every primitive writer, error-prone.

### D2: Emit `<polygon>` instead of `<polyline>`
The SVG renderer SHALL emit a `<polygon>` element for polygon primitives.

- **Alternative A (chosen):** use `<polygon>`. The SVG spec defines `<polygon>` as implicitly closed, so stroked outlines include the closing segment; output keeps the simple `points` attribute; no vertex duplication.
- **Alternative B:** keep `<polyline>` and repeat the first vertex at the end. Works but duplicates data and is easy to forget for future primitives.
- **Alternative C:** emit `<path>` with an explicit `Z` command. Valid but loses the compact `points` form and the existing tests' assertion style.

### D3: Close the Qt path with `closeSubpath()`
The Qt renderer SHALL call `QPainterPath::closeSubpath()` after appending polygon vertices.

- **Alternative A (chosen):** `closeSubpath()` is the idiomatic Qt API; it closes the current subpath, so stroked outlines are closed.
- **Alternative B:** append the first vertex again via `lineTo()`. Duplicates data; relies on vertex memory rather than the path API.

Diagram (border conversion flow, SVG):

```
+-------------+   SetBorder(width mm)   +--------------------------+
| MapPainterSVG| ---------------------> | SymbolRendererSVG        |
+-------------+                        |  strokeWidth =          |
        |                               |    width * screenMmInPixel|
        |                               |  hasStroke = true        |
        |                               +------------|-------------+
        |                                            | DrawPolygon()
        |                                            v
        |                               +--------------------------+
        +--------------------------->  | <polygon points=...>      |
                  SVG stream           |  stroke-width="N"         |
                                       +--------------------------+
```

## Risks / Trade-offs

- [Cairo backend still shows open/stale-width symbol outlines] → Explicitly out of scope; follow-up change on `SymbolRendererCairo`.
- [SVG tests assert exact `stroke-width` strings; factor changes would ripple] → Tests use the factor explicitly passed by the test, not a global default; they are updated together with the behavior (already done in this branch's commit).
- [`closeSubpath()` affects stroked polygons; filled polygons unchanged] → Fill rendering is identical; visual regression limited to stroke-only symbols, where behavior now matches other backends.
- [`<polygon>` change affects `SymbolsAll` tool output] → Tool renders symbols, does not assert element type; no functional impact.

## Migration Plan

None required — rendering fix inside the library; no storage or API format change. Rollback: revert the fix commit.

## Open Questions

None.
