## Context

See proposal.md — Why. `SymbolRendererSVG::SetBorder()` currently leaves the dash branch empty; dashes exist on `BorderStyle` (`HasDashes()`/`GetDash()`) and are emitted by the Cairo backend (`dash[i] * borderWidth`, absolute px) and Qt (dash pattern in pen-width units, scaled internally). SVG output has no dash attribute at all. The comment claiming the interface "doesn't pass dash info" is stale: `SetBorder()` receives the full `BorderStyle`.

## Goals / Non-Goals

**Goals:**
- Dashed symbol borders render in SVG output
- Dash values scale with the converted border width, matching the Cairo backend's semantics

**Non-Goals:**
- No change to solid border output (round-cap intent, existing `stroke-width` behavior)
- No cap-style emission — SVG default (`butt`) already matches the Cairo backend's `CAIRO_LINE_CAP_BUTT`; Qt's flat cap is equivalent
- No changes to other backends

## Decisions

### D1: Scale dash values by the converted border width
Emit `stroke-dasharray="<d0*strokeWidth> <d1*strokeWidth> ..."` where `strokeWidth = width * screenMmInPixel`.

- **Alternative A (chosen):** scale by `strokeWidth` exactly like Cairo scales by `borderWidth`. Identical absolute pixel output for the same style data across backends; consistent with the mm-to-pixel conversion already established for the stroke width.
- **Alternative B:** emit raw dash values unscaled. Would diverge from Cairo's absolute-px semantics; dash spacing would not track the converted border width.
- **Alternative C:** emit dash values in mm. Requires the viewer to understand the symbol coordinate space; no other backend does this.

### D2: Store the dash pattern as renderer state
Compute a formatted dash-array string in `SetBorder()`, store it in a member, and emit it in `WriteFillAndStroke()` when non-empty.

- **Alternative A (chosen):** precomputed string member, emitted next to `stroke-width`. One code path in the stroke writer; solid borders keep `stroke-dasharray` absent.
- **Alternative B:** recompute from `BorderStyle` in the stroke writer. Requires persisting the whole style or screen factor; more state, same result.

### D3: Test via the existing textual assertion pattern
Extend `SymbolRendererSVGTest` with cases asserting the emitted dash array string and its absence for solid borders.

- **Alternative A (chosen):** textual assertions like the existing `SetBorder` cases — the SVG renderer writes to a stream, so output is directly assertable.
- **Alternative B:** pixel-rendered verification via an SVG rasterizer. Unnecessary dependency; output is deterministic text.

## Risks / Trade-offs

- [Dash array scaling mismatch with Qt semantics] → Qt scales internally in pen-width units, which yields the same visual result; Cairo scaling is used as the reference and documented in the spec.
- [Style data uses dash values in mm already] → Style definitions consistently treat dash values relative to border width (as Cairo does); spec locks the scaled behavior.
- [Stale comment in `SetBorder()` confuses future readers] → Removed as part of the change; the interface passes dash info via `BorderStyle`.

## Migration Plan

None — rendering change inside the library; no API or format change. Rollback: revert the commit.

## Open Questions

None.
