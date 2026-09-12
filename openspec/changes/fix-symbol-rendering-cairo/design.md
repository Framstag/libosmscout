## Context

See proposal.md — Why. `SymbolRendererCairo` (libosmscout-map-cairo) already converts border widths (`borderStyle->GetWidth() * screenMmInPixel` in `EndPrimitive()`) and closes polygon paths (`cairo_close_path` in `DrawPolygon()`). Unlike `SymbolRendererSVG`/`SymbolRendererSkia`, it has no unit test. The SVG test verifies behavior via textual output (`<polygon>`, `stroke-width="N"`); Cairo has no textual output, so verification must inspect rasterized pixels. Cairo test linking is proven by `TextMetricsCairoTest` and `MapPainterRouteTest` (link `OSMScout::MapCairo`).

## Goals / Non-Goals

**Goals:**
- Lock Cairo symbol rendering contract with a headless, deterministic unit test
- Register in both CMake and Meson

**Non-Goals:**
- No production code change to `SymbolRendererCairo` unless a test exposes a discrepancy
- No golden-image comparison infrastructure
- No pattern-fill coverage (warns "Pattern is not supported for symbols"; covered by the SymbolsAll tool work on the types branch)

## Decisions

### D1: Verify behavior by pixel sampling on a Cairo image surface
Render each primitive onto a small in-memory image surface (e.g. 32x32, RGB24) and sample pixels through `cairo_image_surface_get_data()` at known coordinates.

- **Alternative A (chosen):** pixel sampling. Deterministic, self-contained, headless (no X server — CI-safe), no new dependencies; asserts the observable contract (pixel colors), exactly what the spec describes.
- **Alternative B:** cairo recording surface + path introspection. Recording surfaces do not rasterize strokes; post-stroke width is not queryable — cannot verify the contract.
- **Alternative C:** golden PNG reference files. Brittle across cairo/antialiasing versions on different CI runners; high maintenance; overkill for a behavioral contract.

Antialiasing handling: sample at pixel centers away from sub-pixel edges (rectangle edges axis-aligned), assert exact colors at interior/gap points; for border-band checks sample the row/column at the band center.

### D2: Mirror the SVG test registration pattern
New `Tests/src/SymbolRendererCairoTest.cpp` registered with `osmscout_test_project(NAME SymbolRendererCairoTest SOURCES ... TARGET OSMScout::Map OSMScout::MapCairo ...)` in `Tests/CMakeLists.txt`, plus the equivalent `executable()` + `test()` block in `Tests/meson.build`.

- **Alternative A (chosen):** reuse the existing macro/meson pattern — identical to `SymbolRendererSVGTest`/`SymbolRendererSkiaTest`; zero new infrastructure.
- **Alternative B:** extend the SVG test binary with Cairo cases. Cross-backend coupling in one binary; wrong failure isolation.

### D3: One test case per spec requirement
Map each spec requirement/scenario to a dedicated `TEST_CASE` (border scaling, closed polygon, fill+stroke, none→background, dashed borders), each asserting sampled pixels.

- **Alternative A (chosen):** explicit cases mirroring scenarios — traceability from tasks to specs is direct (config requires it).
- **Alternative B:** single parametrized mega-test — harder to map to scenarios, worse failure messages.

Diagram (render + verify flow):

```
+------------------+   SetFill/SetBorder/   +----------------------+
| SymbolRenderer-  |   Begin/Draw/End       | cairo image surface  |
| CairoTest        | ---------------------> | 32x32 RGB24          |
+------------------+                        +----------+-----------+
       |                                                |
       | sample pixels (image_surface_get_data)        |
       v                                                v
+------------------+                        +----------------------+
| assertions:      | <----------------------| pixels at known      |
| border band,     |                        | coords (edge,        |
| closing edge,    |                        | interior, gap)       |
| fill, background |                        +----------------------+
+------------------+
```

## Risks / Trade-offs

- [Antialiasing bleeds edge pixels] → Sample at pixel centers, axis-aligned shapes, assert exact colors only at stable points (interior, gap, band center); never at sub-pixel edges.
- [Cairo version differences on CI runners] → Assertions target geometric bands, not sub-pixel positions; ubuntu 24.04 cairo deterministic for solid fills.
- [Test discovery reveals a genuine implementation discrepancy] → Per proposal, fix the implementation in this change; risk is low (behavior verified by inspection) and the outcome is a stricter contract.
- [Dash rendering may be antialiasing-sensitive] → Assert gap pixels show background; avoid asserting dash edge positions.

## Migration Plan

None — test-only change. Rollback: revert the test commit.

## Open Questions

None. Cross-backend dash parity (SVG currently emits no dashes for symbols) is a separate inconsistency, noted but explicitly out of scope.
