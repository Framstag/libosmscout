# Design — Fix Text Metrics and Shield Label Rendering Inconsistencies

## Context

See proposal.md — Why. Findings from analysis (verified with the `TextMetricsAll` demo, Liberation Sans, pixel size 7.56 px, text "Musterstraße 12"):

FreeType reference glyph 0 ("M"): `box=(0, -6, 7, 6)` — per-glyph ink.
Cairo glyph 0 ("M"): `box=(0, -6, 6, 9)` — height 9 for *every* glyph; 9 = font ascent+descent.
Qt glyph 0 ("M"): `box=(0, -5, 7, 5)` — per-glyph ink, close to reference.

Root causes, by file:

1. **Pango ink/logical arguments swapped (Cairo + SVG).**
   `MapPainterCairo.cpp:831` and `MapPainterSVG.cpp:144`:
   ```cpp
   pango_font_get_glyph_extents(font, glyph, nullptr, &extends);
   ```
   `pango_font_get_glyph_extents(font, glyph, ink_rect, logical_rect)` — the code passes `nullptr` as *ink* and reads the *logical* rect, which is constant per font (ascent/descent). Every glyph therefore reports the same font box.

2. **Same swap for whole-layout extents (Cairo).**
   `MapPainterCairo.cpp:815` (`Layout()`) and `:1012`, `:1106` (`DrawLabel()`) use `pango_layout_get_pixel_extents(layout, nullptr, &extends)` — again reading the *logical* rect instead of the ink rect. Consequences: label width/height come from the logical extents, and the `- extends.x` correction in `DrawLabel()` is a no-op (logical x is always 0), so ink bearings are never corrected.

3. **Label height semantics inconsistent across backends.**
   - Cairo+Pango: logical layout extents (asc/desc box)
   - Cairo plain: `textExtents.width` (ink) for width but `fontExtents.height` (font box) for height (`MapPainterCairo.cpp:975-976`)
   - Qt: `naturalTextWidth` × (leading + line height) (font box)
   The label layouter centers label rectangles on the anchor (`LabelLayouter.h:717-719`), so different height semantics place labels differently per backend.

4. **Text draw origin does not match the measured rectangle.**
   - Cairo+Pango `DrawLabel()` (`:1016-1018`): corrects x by `- extends.x` (no-op because logical), ignores the ink y offset → text shifted vertically inside the shield box.
   - Cairo plain (`:1043-1045`): draws with baseline at `rect.y + ascent`, no `x_bearing` correction.
   - Qt: draws at `rect.topLeft()`; consistent with its font-box height semantics but inconsistent with ink metrics.

5. **Shield margins backend-specific.**
   - Cairo (`:1077-1094`): bg `(x-2, y, w+3, h+1)`, border `(x, y+2, w-1, h-3)` — asymmetric, no top padding for the text.
   - Qt (`:360-384`): bg `rect + (-5,-5) + (11,11)`, border `(x+2, y+2, w-3, h-3)` — symmetric ±5 px.
   Result: in Cairo the text glues to the top of the shield; in Qt it floats centered.

Call/graph of label lifecycle (shared pipeline):

```
RegisterRegularLabel ──> LabelLayouter.RegisterLabel
                              │  Layout(projection, parameter, text, ...)   [backend]
                              │  label->width / label->height               [backend semantics differ]
                              v
LabelLayouter.Layout ──> centers rect on anchor: x = px - w/2, y = py - h/2
                              v
LabelLayouter.DrawLabels(router) ──> DrawLabel(projection, parameter, labelRectangle, labelData, nativeLabel)
                                                       │
                              ┌────────────────────────┼────────────────────────┐
                              v                        v                        v
                        text style branch        shield style branch      (special/overlay)
                        draw at rect origin      bg + border around rect  draw at rect origin
                                                 + text at rect origin
                                                                        [backend-specific margins]
```

Compare-and-verify flow (implementation-time validation):

```
TextMetricsAll demo
  ├── FreeType reference (ink per glyph)
  ├── Cairo(+Pango)    ── must now match reference
  ├── Qt               ── must now match reference
  ├── Skia / SVG / AGG ── must now match reference
  └── Cairo(plain, -DHAVE_LIB_PANGO=0 build) ── must now match others
```

## Goals / Non-Goals

**Goals**
- One defined semantics for label measurement: ink extents (matches the existing FreeType reference in `TextMetricsAll` and the `text-metrics-api` spec's "box is tight around drawn glyph" requirement).
- Correct Pango API usage (ink rects) in the Cairo and SVG backends.
- Draw origin consistent with measured rectangle in all cairo text paths and Qt.
- Identical shield margins across backends.
- Both cairo text stacks (Pango / plain cairo) behave identically.

**Non-Goals**
- Changing the `TextMetrics` struct layout or public API signatures.
- Reworking label placement/priority logic in `LabelLayouter`.
- Reusing glyphs vs. per-character extents (perf layout stream) — only per-glyph box *values*.
- Sub-pixel rendering quality or hinting modes (small ±1 px differences from rasterization remain acceptable; tolerances in specs absorb them).
- AGG/Skia/DirectX/GDI/iOSX shield drawing beyond verifying conformance — **updated during implementation (approved scope addition): Skia metrics and glyph boxes were brought to ink semantics (task 3.5)**; AGG and the Windows platforms remain out of scope.
- **AGG note (archive decision)**: AGG is listed in the `text-metrics-api` conformance clause but is not compiled in this workspace/CI environment (`OSMSCOUT_BUILD_MAP_AGG=OFF`, missing deps), so AGG could not be verified. The change restricts automated conformance evidence to Cairo (both stacks), Qt, Skia, and SVG. AGG uses its own FreeType text engine and was not touched by this change; AGG conformance should be verified in a follow-up change when an AGG-enabled build is available.

## Decisions

### D1: Measurement semantics = ink extents

**Decision**: Label `width`/`height` from `Layout()` and the per-glyph boxes from `GlyphBoundingBox()` describe the visual/ink extents. Label rect == union of glyph boxes.

**Alternatives**:
1. *Font-box (typographic) extents everywhere* — chosen today by Qt and plain cairo; simple, but makes measured heights text-independent (a lowercase-only label reserves full asc/desc), shields become oversized, and label centering pulls text toward the top. Rejected: contradicts the existing `text-metrics-api` "tight around drawn glyph" requirement and makes 50%-overlapped labels impossible to reason about tightly.
2. *Logical (layout-aligned) extents* — what cairo+Pango accidentally returns today. Same text-independence problems as the font box plus a constant-value bug.

**Rationale**: Ink semantics is what the layouter needs to center text on an anchor point and what the FreeType reference in `TextMetricsAll` already measures. It makes the "box is the drawing" invariant directly testable.

### D2: Fix Pango calls at the call sites, keep the metric model

**Decision**: Swap `nullptr, &extends` → `&extends, nullptr` in the three Cairo sites (`GlyphBoundingBox()` at :831, `Layout()` at :815, both `DrawLabel()` calls at :1012/:1106) and the SVG site (`MapPainterSVG.cpp:144`). No restructuring of `CairoLabel`/`CairoFont` needed.

**Alternatives**:
1. *Bind cairo-freetype ink extents (`cairo_scaled_font_glyph_extents`) instead* — duplicates the measurement source, diverges from pango's shaping (pango may choose different glyph variants with the same font description). Rejected.
2. *Capture ink extents at layout time and store per-glyph boxes in `CairoLabel`* — more state, bigger diff; `GlyphBoundingBox()` is already the single source. Rejected for now.

**Rationale**: minimal, targeted; both build variants of cairo flow through the same functions, so both stacks are fixed by the same edit.

### D3: Correct the draw origin from ink bearings in `DrawLabel()`/glyph drawing

**Decision**: In every backend's `DrawLabel()` the text is drawn so its ink top-left equals the label rectangle's top-left (plus the shield-internal padding, where applicable):
- Cairo+Pango: correct x **and** y by `- ink.x` / `- ink.y` — ink alignment.
- Cairo plain: position via the string's ink `x_bearing`/`y_bearing` (`cairo_text_extents_t`) instead of assuming the box starts at `ascent`.
- Qt: draw via the existing `textLayout.draw()` but at the ink-corrected origin; since D1 makes Qt metrics ink-based, the correction is `-(ink.x, ink.y)` from `QFontMetrics::tightBoundingRect(text)`.

**Alternatives**:
1. *Keep font-box draw offsets, only fix widths* — leaves the vertical mis-centering (the user-visible bug) untouched. Rejected.
2. *Detect and keep backend-legacy offsets behind a compatibility parameter* — nobody needs the old behavior; no parameter pollution. Rejected.

**Rationale**: with ink semantics (D1) the ink-corrected origin is the only consistent choice; it also makes contour glyph drawing (`DrawGlyphs`) linear off the code being fixed.

### D4: Shared shield margins, defined once in `MapPainter`

**Decision**: Introduce named constants (background padding, border inset) in `MapPainter` (or a small shared helper) and use them in every backend's shield branch: bg = label rect padded by `PAD` (e.g. 2 px) plus border half-width, border inset by `PAD + 1` px — identical numbers in Cairo (both stacks), Qt, Skia, SVG, AGG.

**Alternatives**:
1. *Fixed margins only where broken (Cairo)* — leaves backend-specific numbers in Qt/Skia/SVG; the spec requires identical padding across backends. Rejected.
2. *Move shield drawing wholly into the base class* (bg/border in `MapPainter`, text in the backend) — larger refactor across all backends and their native text objects; worthwhile later, out of scope here. Chosen against for MVP risk reasons; documented as a future refactor.

**Rationale**: single source of truth for the geometry contract in `shield-label-rendering`; minimal surgery per backend.

### D5: Verification harness = `TextMetricsAll` + cross-backend tolerance checks in tests

**Decision**: Keep the existing demo as the cross-stack harness; add regression test cases:
- per-glyph boxes are not constant across glyphs of differing ink (catches the logical-rect swap),
- label rect ≈ union of glyph boxes (catches semantics drift),
- Cairo-Pango vs. cairo-plain build measured in CI (second build job or a `-DHAVE_LIB_PANGO=0` test build) — at minimum, the sanitize/offscreen workflow runs the demo's comparison.

**Alternatives**:
1. *Pixel-diff on rendered shields* — brittle w.r.t. fontconfig versions; used manually, not automated. Rejected for CI.
2. *Golden-image comparison* — same fragility. Rejected.

**Rationale**: matches the project's existing Catch2 pattern (`Tests/src/TextMetricsTest.cpp` etc.) and keeps CI deterministic.

### Sequence of the fixed pipeline (non-trivial flow)

```
Layout(text)
  │  pango_layout_get_pixel_extents  → INK rect        (was: logical)
  │  label->width/height = ink w/h
  v
GlyphBoundingBox(glyph)
  │  pango_font_get_glyph_extents     → INK rect       (was: logical)
  v
MeasureLabel()  → TextMetrics{width,height,glyphs[]}   (ink, backend-equal)
  v
LabelLayouter centers rect on anchor                   (y now centered on true ink)
  v
DrawLabel(shield)
  ├── bg rect   = rect + shared PAD                   (same numbers in all backends)
  ├── border    = rect + shared inset
  └── text      = draw at rect origin − ink bearing   (was: logical no-op x / ascent-only y)
```

## Risks / Trade-offs

- **Visual diff everywhere** (labels will sit ~1-3 px differently than before) → intentional; UI applications regenerate labels each frame; verify in OSMScout2/QML demos and screenshot-compare.
- **Pango ink extents are per-layout; multiline labels** (wrapped) → union of line ink; keep using `pango_layout_get_pixel_extents` ink rect which already aggregates all lines. Verify wrapping labels (`enableWrapping=true`) in demo + Qt app.
- **Qt `tightBoundingRect` cost** — extra metrics call per label; negligible vs. layout cost, but cache within `Layout()` (single call per label).
- **Older Pango versions** — `pango_layout_get_pixel_extents` ink behavior stable across supported versions (used already for SVG); CI covers the shipped distributions.
- **Plain-Cairo path rarely built in CI** — a build without Pango can silently regress; add a CI-consumable build recipe note in tasks (optionally a GitHub Actions job; otherwise document the `HAVE_LIB_PANGO=0` build in `Documentation/`).
- **Contour labels (`DrawGlyphs`)** rely on glyph baseline positions; changing `GlyphBoundingBox()` affects only boxes (collision boxes), positions stay baseline-relative → low regression risk, re-run the router/way-name demo.

## Migration Plan

1. Land metrics fixes (D2, D1 semantics) + draw-origin fixes (D3) per backend in one change — consumers (layouter, shields) read the metrics immediately, so half-states would be visually wrong.
2. Land shield geometry unification (D4).
3. Update/extend tests; run `TextMetricsAll` cross-backend comparison and visual verification in OSMScout2 renderings (Cairo + Qt).
4. Rollback: single-commit revert per backend is safe; no data or API migration involved.

## Open Questions

- Exact shared padding values (proposal: ~2 px like Qt's current border inset, but Qt's current bg pad is 5 px) — decide during implementation against visual comparison; the spec only requires *identical across backends*, not a specific pixel value.
- Whether cairo-no-Pango gets a CI job in this change or only documentation/repro instructions (CI-guard optional).